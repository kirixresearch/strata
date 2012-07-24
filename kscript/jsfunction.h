/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2008-09-10
 *
 */


#ifndef __KSCRIPT_JSFUNCTION_H
#define __KSCRIPT_JSFUNCTION_H


#include "jsobject.h"


namespace kscript
{


class Function : public Object
{
    BEGIN_KSCRIPT_DERIVED_CLASS("Function", Function, Object)
        KSCRIPT_METHOD("constructor", Function::constructor)
        KSCRIPT_METHOD("apply", Function::apply)
        KSCRIPT_METHOD("call", Function::call)
        KSCRIPT_METHOD("toString", Function::toString)
    END_KSCRIPT_CLASS()
    
public:

    Function();
    ~Function();
    
    int invoke(ExprEnv* env,
               Value* vthis,
               Value* retval,
               Value** params,
               size_t param_count);
    
    static void bindStatic(ExprParser* parser);
    static void staticConstructor(ExprEnv* env,
                                  void* param,
                                  Value* retval);
                                  
    void constructor(ExprEnv* env, Value* retval);
    void apply(ExprEnv* env, Value* retval);
    void call(ExprEnv* env, Value* retval);
    void toString(ExprEnv* env, Value* retval);
    
    void toString(Value* retval);
    
public:

    ExprSequence* m_sequence;
    ExprBindFunc m_func_ptr;
    void* m_binding_param;
    bool m_defer_calc;

    std::vector<int> m_param_ids;
    std::wstring m_text;
};




};


#endif

