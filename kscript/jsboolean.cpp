/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2006-06-15
 *
 */


#include "kscript.h"
#include "jsboolean.h"


namespace kscript
{


static Value* getThisBoolean(ExprEnv* env)
{
    Value* ethis = env->getThis();
    if (!ethis)
        return NULL;
        
    if (ethis->isObject())
    {
        ValueObject* vobj = ethis->getObject();
        if (vobj->isKindOf(Boolean::staticGetClassId()))
        {
            return &((Boolean*)vobj)->val;
        }
         else
        {
            return NULL;
        }
    }
    
    return ethis;
}



struct BooleanMethod
{
    const wchar_t* name;
    Value val;
    
    void set(const wchar_t* _name, ExprBindFunc _func)
    {
        name = _name;
        val.setFunction(_func);
    }
};


const int BOOLEAN_METHOD_COUNT = 2;
static BooleanMethod g_boolean_methods[BOOLEAN_METHOD_COUNT];

static void populateBooleanMethods()
{
    if (g_boolean_methods[0].val.isUndefined())
    {
        g_boolean_methods[0].set(L"valueOf",  Boolean::valueOf);
        g_boolean_methods[1].set(L"toString", Boolean::toString);
        // when you add a method, make sure to increment BOOLEAN_METHOD_COUNT above
    }
}





Value* Boolean::lookupBooleanMethod(const std::wstring& method)
{
    populateBooleanMethods();
    
    for (size_t i = 0; i < BOOLEAN_METHOD_COUNT; ++i)
    {
        if (method == g_boolean_methods[i].name)
            return &g_boolean_methods[i].val;
    }
    
    return NULL;
}



// (CLASS) Boolean
// Category: Core
// Derives: Object
// Description: A class that represents boolean-typed data.
// Remarks: A wrapper class for the boolean-typed data.

Boolean::Boolean()
{
    val.setBoolean(false);
}

// (CONSTRUCTOR) Boolean.constructor
// Description: Creates a new Boolean object.
//
// Syntax: Boolean(value : Boolean)
//
// Remarks: Creates a new Boolean object from a primitive boolean value.
//
// Param(value): The |value| of the new Boolean object.

void Boolean::constructor(ExprEnv* env, Value* retval)
{
    val.setBoolean(false);
    
    Value* v = env->getParam(0);
    if (!v->isNull() && !v->isUndefined())
        val.setBoolean(v->getBoolean());
}

void Boolean::staticConstructor(ExprEnv* env,
                                void* param,
                                Value* retval)
{
    Value* v = env->getParam(0);
    if (!v->isNull())
    {
        retval->setBoolean(v->getBoolean());
    }
     else
    {
        retval->setBoolean(false);
    }
}

// (METHOD) Boolean.valueOf
// Description: Returns the primitive boolean value of the Boolean object.
//
// Syntax: function Boolean.valueOf() : Boolean
//
// Remarks: Returns the primitive boolean value represented by the Boolean object.
//
// Returns: Returns the primitive boolean value represented by the Boolean object.

void Boolean::valueOf(ExprEnv* env, void* param, Value* retval)
{
    kscript::Value* vthis = getThisBoolean(env);
    if (!vthis)
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(vthis->getBoolean());
}

// (METHOD) Boolean.toString
// Description: Returns a string representation of the Boolean object.
//
// Syntax: function Boolean.toString() : String
//
// Remarks: Returns a string representation of the Boolean object, which is
//     either "true" or "false".
//
// Returns: Returns either "true" or "false", depending on the value of the Boolean object.

void Boolean::toString(ExprEnv* env, void* param, Value* retval)
{
    kscript::Value* vthis = getThisBoolean(env);
    if (!vthis)
    {
        retval->setString(L"false");
        return;
    }
    
    if (vthis->getBoolean())
        retval->setString(L"true");
         else
        retval->setString(L"false");
}

void Boolean::toPrimitive(Value* retval, int preferred_type)
{
    retval->setBoolean(val.getBoolean());
}


void Boolean::toNumber(Value* retval)
{
    retval->setInteger(val.getBoolean() ? 1 : 0);
}

void Boolean::toString(Value* retval)
{
    if (val.getBoolean())
        retval->setString(L"true");
         else
        retval->setString(L"false");
}


};

