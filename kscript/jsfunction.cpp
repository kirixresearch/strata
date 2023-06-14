/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2008-09-10
 *
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "kscript.h"
#include "elements.h"
#include "jsfunction.h"
#include "util.h"
#include <cmath>


namespace kscript
{


// (CLASS) Function
// Category: Core
// Derives: Object
// Description: A class that represents a function
// Remarks: A class that represents a function

Function::Function()
{
    m_sequence = NULL;
    m_func_ptr = NULL;
    m_binding_param = NULL;
    m_defer_calc = false;
}


Function::~Function()
{
    delete m_sequence;
}

// (CONSTRUCTOR) Function.constructor
// Description: Creates a new Function object.
//
// Syntax: Function()
//
// Remarks: Creates a new Function object from |value|.


void Function::constructor(ExprEnv* env, Value* retval)
{
    size_t i;
    size_t func_param_count = (size_t)env->m_param_count;
    if (func_param_count > 0)
        func_param_count--;
    
    ExprParser* parser = env->getParser();
    ExprParserEnv* penv = env->getParserContext();
    if (!penv)
        return;
    
    baseRef();
    env->getThis()->setFunction(this);
    baseUnref();
    
    ExprParserEnv* new_penv = new ExprParserEnv(penv);
    
    std::vector<std::wstring> params;
    
    // fetch parameter symbol ids
    for (i = 0; i < func_param_count; ++i)
    {
        std::wstring param = env->getParam(i)->getString();
        if (param.find(L",") == param.npos)
        {
            param = xtrim(param);
            if (param.length() > 0)
                params.push_back(param);
        }
         else
        {
            size_t pos;
            while (1)
            {
                pos = param.find(L",");
                if (pos == param.npos)
                {
                    param = xtrim(param);
                    if (param.length() > 0)
                        params.push_back(param);
                    break;
                }
                params.push_back(xtrim(param.substr(0, pos)));
                param = param.substr(pos+1);
            }
        }
    }
    
    std::vector<std::wstring>::iterator it;
    for (it = params.begin(); it != params.end(); ++it)
        m_param_ids.push_back(new_penv->getTokenId(*it));

    if (env->m_param_count > 0)
    {
        Value* code_param = env->m_eval_params[env->m_param_count-1];
        if (!code_param->isString())
            code_param->toString(code_param);
        
        
        wchar_t* expr = new wchar_t[code_param->getDataLen()+1];
        wcscpy(expr, code_param->getString());
        
        parser->removeComments(expr);
        
        parser->m_unresolved.clear();
            
        ExprSequence* e = (ExprSequence*)parser->parseSequence(new_penv, expr, NULL, true);
        
        if (!e)
        {
            delete[] expr;
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
        
        if (!parser->resolveUnresolved())
        {
            delete[] expr;
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
        
        
        // make a textual representation of the function
        m_text = L"function anonymous(";
        for (it = params.begin(); it != params.end(); ++it)
        {
            if (it != params.begin())
                m_text += L", ";
            m_text += *it;
        }
        m_text += L") { ";
        m_text += expr;
        m_text += L" }";


        m_sequence = e;
        delete[] expr;
    }
     else
    {
        m_text = L"function anonymous() { }";
    }
}

void Function::staticConstructor(ExprEnv* env, void*, Value* retval)
{
    Value v;
    Function* f = Function::createObject(env);
    v.setObject(f);
    Value* old_this = env->m_val_this;
    env->m_val_this = &v;
    f->constructor(env, retval);
    env->m_val_this = old_this;
    retval->setValue(v);
}

void Function::bindStatic(ExprParser* parser)
{
}

int Function::invoke(ExprEnv* env,
                     Value* vthis,
                     Value* retval,
                     Value** params,
                     size_t param_count)
{
    int res = evalSucceeded;
    size_t i;
    
    ExprEnv* func_env = env->getParser()->createEnv();
    func_env->setParent(env);
    func_env->setThis(vthis);
    func_env->m_callee = this;

    retval->reset();
    
    
    func_env->reserveParams(param_count);
    func_env->m_param_count = param_count;
    
    for (i = 0; i < param_count; ++i)
    {
        func_env->m_params[i] = params[i];
        
        // evaluate the params if they are not deferred
        if (!m_defer_calc)
        {    
            if (params[i]->eval(env, func_env->m_eval_params[i]) < 0)
            {
                func_env->unref();
                return evalFailed;
            }       
        }
    }
    
    if (m_func_ptr)
    {
        // bound function
        
        func_env->m_binding_param = m_binding_param;
        m_func_ptr(func_env, m_binding_param, retval);
        
        if (env->getRuntimeError())
        {
            res = evalFailed;
        }
         else
        {
            if (retval->isRef())
                retval->getRef()->eval(env, retval);

            res = evalSucceeded;
        }
    }
     else if (m_sequence)
    {
        // JS function

        size_t count = m_param_ids.size();
        for (i = 0; i < count; i++)
        {
            if (i < param_count)
            {
                if (func_env->m_eval_params[i]->eval(env, func_env->declareVar(m_param_ids[i])) < 0)
                {
                    func_env->unref();
                    return evalFailed;
                }
            }
             else
            {
                func_env->declareVar(m_param_ids[i])->setUndefined();
            }
        }

        if (m_sequence->eval(func_env, retval) >= 0)
        {
            res = evalSucceeded;
            if (retval->isRef())
                retval->getRef()->eval(env, retval);
        }
         else
        {
            res = evalFailed;
        }
    }
    
    func_env->unref();
    
    return res;
}



void Function::apply(ExprEnv* env, Value* retval)
{
    Value vthis;
    Value** fparams = NULL;
    size_t fparam_count = 0;
    
    size_t param_count = env->getParamCount();
    if (param_count > 0)
    {
        vthis.setValue(env->getParam(0));
        
        if (param_count > 1)
        {
            Value* arr = env->getParam(1);
            
            size_t i, len = arr->getMember(L"length")->getInteger();
            if (len > 1024)
            {
                env->setRuntimeError(rterrorRangeError);
                return;
            }
                
            
            fparam_count = len;
            fparams = new Value*[fparam_count];
            for (i = 0; i < len; ++i)
            {
                fparams[i] = arr->getMemberI((int)i);
            }
         }
    }

    if (vthis.isNull())
        vthis.setValue(env->getParser()->getGlobalObject());


    invoke(env, &vthis, retval, fparams, fparam_count);

    delete[] fparams;
}

void Function::call(ExprEnv* env, Value* retval)
{
    Value vthis;
    Value** fparams = NULL;
    size_t fparam_count = 0;
    
    size_t i, param_count = env->getParamCount();
    if (param_count > 0)
    {
        vthis.setValue(env->getParam(0));
        
        fparam_count = param_count-1;
        fparams = new Value*[fparam_count];
        for (i = 0; i < fparam_count; ++i)
            fparams[i] = env->getParam(i+1);
    }

    if (vthis.isNull())
        vthis.setValue(env->getParser()->getGlobalObject());


    invoke(env, &vthis, retval, fparams, fparam_count);

    delete[] fparams;
}

// (METHOD) Function.toString
// Description: Returns a string that represents the function source
//
// Syntax: function Function.toString() : String
//
// Remarks: Returns a string that represents the function source
//
// Returns: Returns a string containing the source code for a function

void Function::toString(ExprEnv* env, Value* retval)
{
    toString(retval);
}

void Function::toString(Value* retval)
{
    retval->setString(m_text);
}




};



