/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-10-08
 *
 */


#include "kscript.h"
#include "elements.h"
#include "util.h"
#include "jsboolean.h"
#include "jsarray.h"
#include "jsdate.h"
#include "jserror.h"
#include "jsmath.h"
#include "jsnumber.h"
#include "jsregexp.h"
#include "jsstring.h"
#include "jsobject.h"
#include "jsfunction.h"
#include "json.h"
#include "functions.h"
#include <cctype>
#include <cwctype>
#include <cmath>
#include <cstring>


#ifdef _MSC_VER
#define strcasecmp stricmp
#define wcscasecmp wcsicmp
#define wcsncasecmp wcsnicmp
#endif


#ifdef __APPLE__
#include "../kl/include/kl/portable.h"
#include "../kl/include/kl/string.h"
#define isnan std::isnan
#endif


// remove VC9 warnings - eventually commenting this
// out and implementing specific fixes for these warnings
// is a good idea.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


namespace kscript
{


#define envUseGlobalScope ((ExprEnv*)0x00000000)
#define envUseCallerScope ((ExprEnv*)0x00000001)


// utility functions

inline bool failed(int x)
{
    return (x < 0) ? true : false;
}

inline bool succeeded(int x)
{
    return (x >= 0) ? true : false;
}

static bool nextTokenIs(const wchar_t* str, const wchar_t* search, int len)
{
    if (0 == wcsncmp(str, search, len) &&
        (isWhiteSpaceOrLS(*(str+len)) || wcschr(L"()[]{}:;+-!~./\"\'", *(str+len))))
    {
        return true;
    }

    return false;
}


const static std::wstring g_empty_string;

// constructor function for objects

class ClassConstructorRef : public ExprElement
{
public:

    ClassConstructorRef()
    {
        m_function = NULL;
        m_info = NULL;
    }

    int eval(ExprEnv* env, Value* retval)
    {
        if (m_info)
            retval->setValue(m_info->m_class_obj);
             else
            retval->setFunction(m_function);
        return evalSucceeded;
    }

    int getType() { return Value::typeFunction; }
    Value* getLvalue(ExprEnv* env) { return NULL; }

public:

    Function* m_function;
    ExprClassInfo* m_info;
};

static void kscript_object_constructor(ExprEnv* env,
                                       void* param,
                                       Value* retval)
{
    ExprClassInfo* info = (ExprClassInfo*)param;
    
    
    bool object_mutate = false;
    
    
    // find a object initializer, if any
    ExprClassInfo* curinfo = info;
    while (curinfo)
    {
        if (curinfo->m_objinit_func)
        {
            curinfo->m_objinit_func(env, (void*)&object_mutate, retval);
            break;
        }
        
        curinfo = curinfo->getBaseClass();
    }
    
    if (!env->getThis())
        return;
    

    // some built-in JS objects mutate.  For instance, the code
    // "new Object(1.01)" will actually create a Number object.
    // This is dealt with here by this slight-of-hand, the objects
    // are already prepared, so a constructor call is not necessary
    
    if (object_mutate)
        return;

    
    Value* vthis = env->getThis();
    if (!vthis->isObject())
    {
        vthis->setObject(Object::createObject(env));
    }


    ValueObject* obj = vthis->getObject();

    obj->setParser(env->getParser());
    obj->setPrototype(&info->m_prototype);
    info->setupObject(obj);

    curinfo = info;
    while (curinfo)
    {
        if (curinfo->m_cstdef)
        {
            Value v;
            v.setFunction(curinfo->m_cstdef);
            env->getParser()->invoke(env, &v, vthis, retval, env->m_eval_params, env->m_param_count);
            break;
        }
        
        curinfo = curinfo->getBaseClass();
    }

    
    ClassConstructorRef* cstref = new ClassConstructorRef;
    cstref->m_info = info;
    env->getThis()->createMember(L"constructor", Value::attrDontEnum)->setExprElement(cstref);
}


// ExprEnv implementation

ExprEnv::ExprEnv()
{
    m_ref_count = 0;
    m_parent = NULL;
    m_parser = NULL;
    m_parser_env = NULL;
    m_val_this = NULL;
    m_val_with = NULL;
    m_valreturn_this = NULL;
    m_valreturn_arrinfo = NULL;
    m_newobj = NULL;
    m_switchcase_offset = 0;
    m_param_count = 0;
    m_param_array_size = 0;
    m_params = NULL;
    m_arguments = NULL;
    m_eval_params = NULL;
    m_binding_param = NULL;
    m_prefix = false;
    m_min_var = -1;
    m_max_var = -1;
    m_null_value = NULL;
    m_callee = NULL;
}

ExprEnv::~ExprEnv()
{
    m_ref_count = 20000000;
    m_parser = NULL;
        
    for (size_t i = 0; i < m_param_array_size; ++i)
        delete m_eval_params[i];
    
    delete[] m_params;
    delete[] m_eval_params;
    delete m_null_value;
    delete m_arguments;
}


int ExprEnv::getParamType(size_t param)
{
    if (param > m_param_count)
        return Value::typeUndefined;
    
    return m_eval_params[param]->getType();
}
    
Value* ExprEnv::getParam(size_t param)
{
    if (param >= m_param_count)
    {
        if (!m_null_value)
            m_null_value = new Value;
        
        m_null_value->reset();
        return m_null_value;
    }
        
    return m_eval_params[param];
}


void ExprEnv::reset()
{    
    if (m_min_var >= 0)
    {
        int i;
        for (i = m_min_var; i <= m_max_var; ++i)
        {
            delete m_vars[i];
            m_vars[i] = NULL;
        }

        m_min_var = -1;
        m_max_var = -1;
    }

    if (m_parent)
    {
        m_parent->unref();
        m_parent = NULL;
    }
        
    m_parser_env = NULL;
    m_val_this = NULL;
    m_val_with = NULL;
    m_valreturn_this = NULL;
    m_valreturn_arrinfo = NULL;
    m_newobj = NULL;
    m_switchcase_offset = 0;
    m_param_count = 0;
    m_binding_param = NULL;
    m_prefix = false;
    delete m_arguments;
    m_arguments = NULL;
    m_callee = NULL;

    for (size_t i = 0; i < m_param_array_size; ++i)
    {
        m_params[i] = NULL;
        
        if (m_eval_params[i] && m_eval_params[i]->hasObject())
            m_eval_params[i]->reset();
    }
}


void ExprEnv::resetCircular()
{
    if (m_min_var >= 0)
    {
        int i;
        for (i = m_min_var; i <= m_max_var; ++i)
        {
            if (m_vars[i] && m_vars[i]->isFunction())
                m_vars[i]->releaseScope();
        }
    }
}

void ExprEnv::reserveVars(size_t size)
{
    m_vars.reserve(size);
}

Value* ExprEnv::declareVar(int var_id)
{
    if (var_id >= (int)m_vars.size())
        m_vars.resize(var_id+10);
    
    if (m_vars[var_id] == NULL)
    {
        if (m_min_var < 0 || var_id < m_min_var)
            m_min_var = var_id;
        if (m_max_var < var_id)
            m_max_var = var_id;

        Value* v = new Value;
        m_vars[var_id] = v;
        return v;
    }
    
    return m_vars[var_id];
}

Value* ExprEnv::getValue(int var_id)
{
    if (var_id >= (int)m_vars.size() || !m_vars[var_id])
    {
        if (!m_parent)
            return NULL;
        return m_parent->getValue(var_id);
    }

    return m_vars[var_id];
}


ExprParserEnv* ExprEnv::getParserContext()
{
    ExprEnv* env_it = this;
    while (env_it)
    {
        if (env_it->m_parser_env)
            return env_it->m_parser_env;
        env_it = env_it->m_parent;
    }
    
    // couldn't find one
    return NULL;
}

bool ExprEnv::createObject(const std::wstring& class_name,
                           kscript::Value* obj)
{
    if (!m_parser)
    {
        obj->setNull();
        return false;
    }

    return m_parser->createObject(class_name, obj);
}

void ExprEnv::reserveParams(size_t param_count)
{
    size_t i;
    
    // make sure we have enough space in the arrays
    if (param_count <= m_param_array_size)
        return;

    ExprElement** params = new ExprElement*[param_count];
    Value** eval_params = new Value*[param_count];

    for (i = 0; i < param_count; ++i)
    {
        params[i] = NULL;
        eval_params[i] = NULL;
    }
    
    // copy old data into the new array and
    // initialize new array elements
    for (i = 0; i < param_count; ++i)
    {
        if (i < m_param_array_size)
            eval_params[i] = m_eval_params[i];
             else
            eval_params[i] = new Value;
    }
    
    // delete the old arrays
    delete[] m_params;
    delete[] m_eval_params;
    
    m_params = params;
    m_eval_params = eval_params;
    
    // set new array size
    m_param_array_size = param_count;
}



// ExprEmptyStatement class implementation

ExprEmptyStatement::ExprEmptyStatement()
{
}

ExprEmptyStatement::~ExprEmptyStatement()
{
}

int ExprEmptyStatement::eval(ExprEnv* env, Value* retval)
{
    return errorNone;
}

int ExprEmptyStatement::getType()
{
    return Value::typeUndefined;
}




// ExprNew class implementation

ExprNew::ExprNew()
{
    m_expr = NULL;
}

ExprNew::~ExprNew()
{
    delete m_expr;
}

int ExprNew::eval(ExprEnv* env, Value* retval)
{
    Value thisobj; // this will be our |this|, which 'new' will return
    Value r; // throw-away return value (constructors don't return anything)
    
    env->m_newobj = &thisobj;
    int res = m_expr->eval(env, &r);
    env->m_newobj = NULL;
        
    // return of new operator is |this|
    thisobj.eval(NULL, retval);

    return res;
}

int ExprNew::getType()
{
    return m_expr->getType();
}




// ExprVarDefine implementation

ExprVarDefine::ExprVarDefine()
{
    m_scope = NULL;
    m_init = NULL;
}

ExprVarDefine::~ExprVarDefine()
{
    delete m_init;
}

int ExprVarDefine::eval(ExprEnv* env, Value* retval)
{
    Value* v;
    
    
    if (m_scope)
    {
        // find highest environment which uses this parser context
        ExprEnv* target_env = NULL;
        ExprEnv* e = env;
        while (e)
        {
            if (e->m_parser_env == m_scope)
                target_env = e;
            e = e->m_parent;
        }
        
        if (!target_env)
            target_env = env;
        
        v = target_env->declareVar(m_var_id);
    }
     else
    {
        v = env->declareVar(m_var_id);
    }
    
    v->setAttributes(Value::attrDontDelete);
    

    if (m_init)
    {
        if (failed(m_init->eval(env, v)))
            return evalFailed;
    }

    // TODO: revisit this.  put a "var a;" in a constructor...'this'
    // will get overwritten
    //retval->setUndefined();
    return evalSucceeded;
}

int ExprVarDefine::getType()
{
    return Value::typeUndefined;
}



// ExprVarAssign implementation

ExprVarAssign::ExprVarAssign()
{
    m_init = NULL;
    m_var_id = 0;
}

ExprVarAssign::~ExprVarAssign()
{
    delete m_init;
}

int ExprVarAssign::eval(ExprEnv* env, Value* retval)
{
    Value* v;
    v = env->getValue(m_var_id);
    if (m_init && v)
    {
        if (failed(m_init->eval(env, v)))
            return evalFailed;
    }

    return evalSucceeded;
}

int ExprVarAssign::getType()
{
    return Value::typeUndefined;
}




ExprSimpleArrayLookup::ExprSimpleArrayLookup()
{
    m_left = NULL;
}

ExprSimpleArrayLookup::~ExprSimpleArrayLookup()
{
    delete m_left;
}

int ExprSimpleArrayLookup::eval(ExprEnv* env, Value* retval)
{
    ExprOperArrayInfo* arrinfo = env->m_valreturn_arrinfo;
    env->m_valreturn_arrinfo = NULL;
    
    Value* leftp;
    Value* val;
    
    if (env->m_valreturn_this)
    {
        leftp = env->m_valreturn_this;
        env->m_valreturn_this = NULL;
        if (failed(m_left->eval(env, leftp)))
            return evalFailed;
    }
     else
    {
        leftp = m_left->getLvalue(env);
        if (leftp && leftp->isExprElement())
            leftp = NULL;
    }
    
    if (leftp)
    {
        if (arrinfo)
        {
            arrinfo->left.setValue(leftp);
            arrinfo->right = m_right;
            if (arrinfo->noeval)
            {
                retval->reset();
                return evalSucceeded;
            }
        }
        
        
        val = leftp->getMember(m_right);
        if (val)
            return val->eval(env, retval);
             else            
            retval->setUndefined();
    }
     else
    {
        Value left;
        if (failed(m_left->eval(env, &left)))
            return evalFailed;
        
        if (arrinfo)
        {
            arrinfo->left.setValue(&left);
            arrinfo->right = m_right;
            if (arrinfo->noeval)
            {
                retval->reset();
                return evalSucceeded;
            }
        }
        
        val = left.getMember(m_right);
        if (val)
            return val->eval(env, retval);
             else            
            retval->setUndefined();
    }
    
    return evalSucceeded;
}


Value* ExprSimpleArrayLookup::getLvalue(ExprEnv* env)
{
    if (!env)
        return NULL;
    
    ExprOperArrayInfo* arrinfo = env->m_valreturn_arrinfo;
    env->m_valreturn_arrinfo = NULL;

    Value* leftp;
    Value* val;
    
    if (env->m_valreturn_this)
    {
        leftp = env->m_valreturn_this;
        env->m_valreturn_this = NULL;
        if (failed(m_left->eval(env, leftp)))
            return NULL;
    }
     else
    {
        leftp = m_left->getLvalue(env);
        if (leftp && leftp->isExprElement())
            leftp = NULL;
    }
    
    if (leftp)
    {
        if (arrinfo)
        {
            arrinfo->left.setValue(leftp);
            arrinfo->right = m_right;
            if (arrinfo->noeval)
                return NULL;
        }
        
        val = leftp->getMember(m_right);

        return val;
    }
     else
    {
        Value left;
        if (failed(m_left->eval(env, &left)))
            return NULL;
        
        if (!arrinfo && left.getObject()->m_base_ref_count <= 1)
        {
            // object won't survive deref
            return NULL;
        }
        
        if (arrinfo)
        {
            arrinfo->left.setValue(&left);
            arrinfo->right = m_right;
            if (arrinfo->noeval)
                return NULL;
        }
        
        return left.getMember(m_right);
    }
    
    return NULL;
}

int ExprSimpleArrayLookup::getType()
{
    return Value::typeUndefined;
}





// ExprFunctionInit implementations

ExprFunctionInit::ExprFunctionInit(Function* func)
{
    m_function = func;
    m_function->baseRef();
}

ExprFunctionInit::~ExprFunctionInit()
{
    m_function->baseUnref();
}


int ExprFunctionInit::eval(ExprEnv* env, Value* retval)
{
    retval->setFunction(m_function, env);
    
    return evalSucceeded;
}

int ExprFunctionInit::getType()
{
    return Value::typeFunction;
}






// ExprArrayInit implementations

ExprArrayInit::ExprArrayInit()
{
}

ExprArrayInit::~ExprArrayInit()
{
    std::vector<ExprElement*>::const_iterator it, it_end = m_elements.end();
    for (it = m_elements.begin(); it != it_end; ++it)
        delete (*it);
}


int ExprArrayInit::eval(ExprEnv* env, Value* retval)
{
    retval->setObject(Array::createObject(env));

    std::vector<ExprElement*>::iterator it;
    int idx = 0;

    for (it = m_elements.begin(); it != m_elements.end(); ++it)
    {
        int res = (*it)->eval(env, retval->getMemberI(idx++));

        if (failed(res))
            return res;
    }

    return evalSucceeded;
}

int ExprArrayInit::getType()
{
    return Value::typeObject;
}



// ExprObjectLiteralInit implementations

ExprObjectLiteralInit::ExprObjectLiteralInit()
{
}

ExprObjectLiteralInit::~ExprObjectLiteralInit()
{
    std::vector<ExprObjectLiteralItem>::const_iterator it, it_end = m_elements.end();
    for (it = m_elements.begin(); it != it_end; ++it)
        delete it->m_varinit;
}


int ExprObjectLiteralInit::eval(ExprEnv* env, Value* retval)
{
    retval->setObject(Object::createObject(env));
    
    int res;
    std::vector<ExprObjectLiteralItem>::iterator it;
    for (it = m_elements.begin(); it != m_elements.end(); ++it)
    {
        res = it->m_varinit->eval(env, retval->getMember(it->m_name));

        if (failed(res))
            return res;
    }

    return evalSucceeded;
}

int ExprObjectLiteralInit::getType()
{
    return Value::typeObject;
}






// ExprThis implementation

ExprThis::ExprThis()
{
}

int ExprThis::eval(ExprEnv* env, Value* retval)
{
    Value* v = env->getThis();
    if (!v)
    {
        retval->setValue(env->getParser()->getGlobalObject());
        //retval->setNull();
        return evalSucceeded;
    }
    
    return v->eval(env, retval);
}

int ExprThis::getType()
{
    return Value::typeUndefined;
}

Value* ExprThis::getLvalue(ExprEnv* env)
{
    if (!env)
        return NULL;
    return env->getThis();
}





// ExprSuper implementation

ExprSuper::ExprSuper()
{
    m_base_class_info = NULL;
}

ExprSuper::~ExprSuper()
{

}

int ExprSuper::eval(ExprEnv* env, Value* retval)
{
    Value* v = getLvalue(env);
    return v->eval(env, retval);
}

int ExprSuper::getType()
{
    return Value::typeFunction;
}

Value* ExprSuper::getLvalue(ExprEnv* env)
{
    if (m_val.getType() != Value::typeFunction)
    {
        // we need to return a function pointer
        // to either a base class constructor
        // or, if none exists, a null function ptr

        if (m_base_class_info && m_base_class_info->m_cstdef)
        {
            m_val.setFunction(m_base_class_info->m_cstdef);
        }
         else
        {
            m_val.setFunction(env->getParser()->createFunction());
        }
    }
    
    if (env && env->m_valreturn_this)
        env->m_valreturn_this->setValue(env->getThis());

    return &m_val;
}




// ExprVarLookup implementation

ExprVarLookup::ExprVarLookup()
{
    m_mode = modeVarId;
    m_var_id = 0;
    m_retval_type = Value::typeUndefined;
    
    m_parser = NULL;
    m_scope = NULL;
    m_location = NULL;
}

ExprVarLookup::~ExprVarLookup()
{
    if (m_parser)
        m_parser->removeUnresolved(this);
}


int ExprVarLookup::eval(ExprEnv* env, Value* retval)
{
    if (env->m_val_with)
    {
        // 'with' mode
        if (env->m_val_with->getMemberExists(m_symbol))
        {
            env->m_val_with->getMember(m_symbol)->eval(env, retval);
            return evalSucceeded;
        }
    }

    if (m_mode == modeVarId)
    {
        Value* v = env->getValue(m_var_id);

        if (!v)
        {
            retval->setUndefined();
            return evalSucceeded;
        }

        return v->eval(env, retval);
    }
     else if (m_mode == modeObjectMember)
    {
        Value* this_val = env->getThis();
        
        if (this_val)
        {
            this_val->getMember(m_symbol)->eval(env, retval);
        }
         else
        {
            retval->setNull();
        }
    }
     else if (m_mode == modeRuntime)
    {
        Value* v = env->getValue(m_var_id);
        
        if (v)
        {
            return v->eval(env, retval);
        }
         else
        {
            // variable doesn't exist in any scope
            // look up global object member
            Value* global = env->getParser()->getGlobalObject();
            if (!global->getMemberExists(m_symbol))
            {
                env->setRuntimeError(rterrorReferenceError, NULL, m_symbol);
                return evalFailed;
            }
            
            return global->getMember(m_symbol)->eval(env, retval);
        }
    }
     else
    {
        // unknown mode
        return evalFailed;
    }
    
    return evalSucceeded;
}

int ExprVarLookup::getType()
{
    return m_retval_type;
}

Value* ExprVarLookup::getLvalue(ExprEnv* env)
{
    if (!env)
        return NULL;

    if (env->m_val_with)
    {
        // 'with' mode
        if (env->m_val_with->getMemberExists(m_symbol))
        {
            return env->m_val_with->getMember(m_symbol);
        }
    }
            
    if (m_mode == modeVarId)
    {
        return env->getValue(m_var_id);
    }
     else if (m_mode == modeObjectMember)
    {
        Value* this_val = env->getThis();
        
        if (this_val)
        {
            if (env->m_valreturn_this)
                env->m_valreturn_this->setValue(this_val);
                
            return this_val->getMember(m_symbol);
        }
         else
        {
            return NULL;
        }
    }
     else if (m_mode == modeRuntime)
    {
        if (m_var_id == 0)
        {
            m_var_id = env->getParser()->getSymbolId(m_symbol);
        }
        
        if (m_var_id != 0)
        {
            Value* v = env->getValue(m_var_id);
            if (v)
                return v;
        }
        
        
        
        
        Value* global = env->getParser()->getGlobalObject();
        
        ExprOperArrayInfo* arrinfo = env->m_valreturn_arrinfo;
        env->m_valreturn_arrinfo = NULL;
        
        if (arrinfo)
        {
            arrinfo->left.setValue(global);
            arrinfo->right = m_symbol;
        }
            
        return global->getMember(m_symbol);
    }
     else
    {
        return NULL;
    }
}



// ExprWith implementation

ExprWith::ExprWith()
{
    m_with = NULL;
    m_sequence = NULL;
}

ExprWith::~ExprWith()
{
    delete m_with;
    delete m_sequence;
}

int ExprWith::eval(ExprEnv* env, Value* retval)
{
    if (!m_sequence)
        return evalSucceeded;
    
    ExprEnv* with_env = env->m_parser->createEnv();
    with_env->setParent(env);
    with_env->m_val_this = env->m_val_this;
    with_env->m_valreturn_this = env->m_valreturn_this;
    with_env->m_valreturn_arrinfo = env->m_valreturn_arrinfo;
    
    int res;
    
    Value vwith;
    res = m_with->eval(env, &vwith);
    if (failed(res))
    {
        with_env->unref();
        return res;
    }
    
    with_env->m_val_with = &vwith;
    
    res = m_sequence->eval(with_env, retval);
    
    with_env->m_val_with = NULL;
    with_env->unref();
    
    return res;
}

int ExprWith::getType()
{
    if (m_sequence)
        return m_sequence->getType();
         else
        return Value::typeUndefined;
}




// ExprIf implementation

ExprIf::ExprIf()
{
    m_condition = NULL;
    m_iftrue = NULL;
    m_iffalse = NULL;
}

ExprIf::~ExprIf()
{
    if (m_condition)
    {
        delete m_condition;
    }

    if (m_iftrue)
    {
        delete m_iftrue;
    }

    if (m_iffalse)
    {
        delete m_iffalse;
    }
}

int ExprIf::eval(ExprEnv* env, Value* retval)
{
    Value res;
    if (failed(m_condition->eval(env, &res)))
        return evalFailed;

    if (res.getBoolean())
    {
        return m_iftrue->eval(env, retval);
    }

    if (m_iffalse)
    {
        return m_iffalse->eval(env, retval);
    }

    return evalSucceeded;
}

int ExprIf::getType()
{
    return Value::typeUndefined;
}



// ExprForWhile implementation

ExprForWhile::ExprForWhile()
{
    m_init = NULL;
    m_condition = NULL;
    m_loop_oper = NULL;
    m_sequence = NULL;
    m_condition_at_end = false;
    m_label_id = 0;
}

ExprForWhile::~ExprForWhile()
{
    delete m_init;
    delete m_condition;
    delete m_loop_oper;
    delete m_sequence;
}


int ExprForWhile::eval(ExprEnv* env, Value* retval)
{
    if (m_init)
    {
        Value v;
        if (failed(m_init->eval(env, &v)))
            return evalFailed;
    }

    Value condres;
    Value res;
    int result;
    
    while (1)
    {
        if (!m_condition_at_end && m_condition)
        {
            if (failed(m_condition->eval(env, &condres)))
                return evalFailed;
            if (!condres.getBoolean())
                break;
        }

        result = m_sequence->eval(env, retval);

        if (result != evalSucceeded)
        {
            if (failed(result))
                return evalFailed;
             else if (result == evalSucceededReturn)
                return evalSucceededReturn;
             else if (result == evalSucceededBreak)
            {
                int label = env->getBreakContinueLabel();
                if (label == 0 || label == m_label_id)
                    return evalSucceeded;
                     else
                    return evalSucceededBreak;
            }
             else if (result == evalSucceededContinue)
            {
                int label = env->getBreakContinueLabel();
                if (label != 0 && label != m_label_id)
                    return evalSucceededContinue;
            }
        }

        if (m_loop_oper)
        {
            if (failed(m_loop_oper->eval(env, &res)))
                return evalFailed;
        }
        
        if (m_condition_at_end && m_condition)
        {
            if (failed(m_condition->eval(env, &condres)))
                return evalFailed;
            if (!condres.getBoolean())
                break;
        }
    }


    return evalSucceeded;
}

int ExprForWhile::getType()
{
    return Value::typeUndefined;
}



// ExprForIn implementation

ExprForIn::ExprForIn()
{
    m_init = NULL;
    m_element = NULL;
    m_arr = NULL;
    m_sequence = NULL;
    m_label_id = 0;
}

ExprForIn::~ExprForIn()
{
    delete m_init;
    delete m_element;
    delete m_arr;
    delete m_sequence;
}


int ExprForIn::eval(ExprEnv* env, Value* retval)
{
    if (m_init)
    {
        Value v;
        if (failed(m_init->eval(env, &v)))
            return evalFailed;
    }

    Value* arrv = m_arr->getLvalue(env);
    if (!arrv)
        return evalFailed;
    
    if (!arrv->isObject())
    {
        // value is not an object or array,
        // so no iterations are necessary
        return evalSucceeded;
    }
    
    ValueObject* arr = arrv->getObject();
    

    Value* element = m_element->getLvalue(env);
    if (!element)
        return evalFailed;

    size_t len = arr->getRawMemberCount();
    size_t i;
    int result;
    
    for (i = 0; i < len; ++i)
    {
        if (!element->getReadOnly())
            element->setString(arr->getRawMemberName(i).c_str());
        
        result = m_sequence->eval(env, retval);
        
        if (result != evalSucceeded)
        {
            if (failed(result))
                return evalFailed;
             else if (result == evalSucceededReturn)
                return evalSucceededReturn;
             else if (result == evalSucceededBreak)
            {
                int label = env->getBreakContinueLabel();
                if (label == 0 || label == m_label_id)
                    return evalSucceeded;
                     else
                    return evalSucceededBreak;
            }
             else if (result == evalSucceededContinue)
            {
                int label = env->getBreakContinueLabel();
                if (label != 0 && label != m_label_id)
                    return evalSucceededContinue;
            }
        }
    }

    return evalSucceeded;
}

int ExprForIn::getType()
{
    return Value::typeUndefined;
}




// ExprSwitch implementation

ExprSwitch::ExprSwitch()
{
    m_default_count = 0;
    m_label_id = 0;
    m_expr = NULL;
    m_sequence = NULL;
}

ExprSwitch::~ExprSwitch()
{
    delete m_expr;
    delete m_sequence;
    
    std::vector<ExprSwitchCase>::iterator it;
    for (it = m_cases.begin(); it != m_cases.end(); ++it)
        delete it->case_element;
}


int ExprSwitch::eval(ExprEnv* env, Value* retval)
{
    retval->setUndefined();
    
    int eval_res = evalSucceeded;
    Value test_res;
    
    Value left;
    Value right;
    
    if (failed(m_expr->eval(env, &left)))
        return evalFailed;
    
    ExprEnv tempenv;
    Value* params[2];
    params[0] = &left;
    params[1] = &right;
    tempenv.m_parser = env->m_parser;
    tempenv.m_eval_params = params;
    tempenv.m_param_count = 2;
    
    ExprSwitchCase* case_to_run = NULL;
    ExprSwitchCase* default_case = NULL;
    
    std::vector<ExprSwitchCase>::iterator it;
    for (it = m_cases.begin(); it != m_cases.end(); ++it)
    {
        if (it->case_element == NULL)
        {
            default_case = &(*it);
            continue;
        }
        
        if (failed(it->case_element->eval(env, &right)))
            return evalFailed;

        oper_strict_equals(&tempenv, NULL, &test_res);
        
        if (test_res.getBoolean())
        {
            case_to_run = &(*it);
            break;
        }
    }
    
    if (!case_to_run)
        case_to_run = default_case;
        
    if (case_to_run)
    {
        // we found a match, so we don't need to evaluate the default
        default_case = NULL;
        
        size_t old_off = env->m_switchcase_offset;
        env->m_switchcase_offset = case_to_run->sequence_offset;
        eval_res = m_sequence->eval(env, retval);
        env->m_switchcase_offset = old_off;
        
        // evalSucceededBreak breaks us out of the loop, but
        // we should propogate this value any further upstream;
        // all other values, such as continue or return, get propogated
        if (eval_res == evalSucceededBreak)
        {
            int label_id = env->getBreakContinueLabel();
            if (label_id == 0 || label_id == m_label_id)
                eval_res = evalSucceeded;
        }
    }
    
    tempenv.m_param_count = 0;
    tempenv.m_eval_params = 0;
    
    return eval_res;
}

int ExprSwitch::getType()
{
    return Value::typeUndefined;
}


// ExprInstruction implementation

ExprInstruction::ExprInstruction()
{
    m_params = NULL;
    m_eval_params = NULL;
    m_retval_type = Value::typeUndefined;
    m_defer_calc = false;
    m_prefix = false;
}

ExprInstruction::~ExprInstruction()
{
    int i;

    if (m_params)
    {
        for (i = 0; i < m_param_count; ++i)
            delete m_params[i];
        
        delete[] m_params;
    }

    if (m_eval_params)
    {
        for (i = 0; i < m_param_count; ++i)
            delete m_eval_params[i];

        delete[] m_eval_params;
    }
}

int ExprInstruction::getType()
{
    return m_retval_type;
}


int ExprInstruction::eval(ExprEnv* env, Value* retval)
{
    if (env->m_parser->m_language == optionLanguageECMAScript)
    {
        // this version is required for recursive calls

        ExprEnv* func_env = env->m_parser->createEnv();
        func_env->setParent(env);
        func_env->m_val_this = env->m_val_this;
        func_env->m_val_with = env->m_val_with;
        func_env->m_valreturn_this = env->m_valreturn_this;
        func_env->m_valreturn_arrinfo = env->m_valreturn_arrinfo;
        func_env->m_prefix = m_prefix;
        func_env->reserveParams(m_param_count);
        
        int i;
        for (i = 0; i < m_param_count; i++)
        {
            func_env->m_params[i] = m_params[i];
        
            if (!m_defer_calc)
            {
                int res = m_params[i]->eval(env, func_env->m_eval_params[i]);
                if (failed(res))
                {
                    func_env->unref();
                    return res;
                }
            }
        }

        
        m_func_ptr(func_env, NULL, retval);
        
    
        if (func_env->getRuntimeError())
        {
            // runtime error encountered
            func_env->unref();
            return evalFailed;
        }
         

        if (retval->isRef())
        {
            retval->getRef()->eval(env, retval);
        }
        
        func_env->unref();

        return evalSucceeded;
    }
     else
    {
        // this version is used for simple expressions
        // which will never be inside recursive functions

        if (!m_defer_calc)
        {
            for (int i = 0; i < m_param_count; i++)
            {
                m_params[i]->eval(env, m_eval_params[i]);
            }
        }
    
        ExprElement** old_p = env->m_params;
        Value** old_v = env->m_eval_params;
        
        env->m_param_count = m_param_count;
        env->m_params = m_params;
        env->m_eval_params = m_eval_params;
        env->m_prefix = m_prefix;

        m_func_ptr(env, NULL, retval);

        env->m_param_count = 0;
        env->m_eval_params = old_v;
        env->m_params = old_p;
        env->m_prefix = false;

        if (env->getRuntimeError())
        {
            // runtime error encountered
            return evalFailed;
        }

        if (retval->isRef())
        {
            retval->getRef()->eval(env, retval);
        }

        return evalSucceeded;
    }
}


Value* ExprInstruction::getLvalue(ExprEnv* env)
{
    if (!env)
        return NULL;
    
    ExprEnv* func_env = env->m_parser->createEnv();
    func_env->setParent(env);
    func_env->m_val_this = env->m_val_this;
    func_env->m_val_with = env->m_val_with;
    func_env->m_valreturn_this = env->m_valreturn_this;
    func_env->m_valreturn_arrinfo = env->m_valreturn_arrinfo;
    func_env->reserveParams(m_param_count);
    
    int i;
    for (i = 0; i < m_param_count; i++)
    {
        func_env->m_params[i] = m_params[i];
        
        if (!m_defer_calc)
        {
            if (failed(m_params[i]->eval(env, func_env->m_eval_params[i])))
            {
                func_env->unref();
                return NULL;
            }
        }
    }

    Value retval;
    m_func_ptr(func_env, NULL, &retval);
    
    
    func_env->unref();

    if (retval.isRef())
    {
        return retval.getRef();
    }

    return NULL;
}




// ExprFunctionCall implementation

ExprFunctionCall::ExprFunctionCall()
{
    m_params = NULL;
    m_func_ptr = NULL;
}

ExprFunctionCall::~ExprFunctionCall()
{
    if (m_params)
    {
        for (size_t i = 0; i < m_param_count; i++)
            delete m_params[i];

        delete[] m_params;
    }
    
    delete m_func_ptr;
}

int ExprFunctionCall::eval(ExprEnv* env, Value* retval)
{
    ExprDepthCounter cnt(&env->m_parser->m_current_depth);
    if (env->m_parser->m_current_depth >= env->m_parser->m_max_depth)
    {
        env->setRuntimeError(rterrorRangeError);
        return evalFailed;
    }


    Value val_this;
    val_this.m_type = 255;
    env->m_valreturn_this = &val_this;

    Value* val_func = NULL;
    Value* val = m_func_ptr->getLvalue(env);
    Function* func_info = NULL;
    ExprEnv* func_scope = NULL;
    
    if (!val || val->getType() != Value::typeFunction)
    {
        val_func = new Value;
        m_func_ptr->eval(env, val_func);
        env->m_valreturn_this = NULL;
        
        if (val_func->isFunction())
        {
            func_info = val_func->getFunction();
            func_scope = val_func->getFunctionScope();
        }
        
        if (func_info == NULL)
        {
            if (val && val->getMemberExists(L"toFunction"))
            {
                Value* func = val->getMember(L"toFunction");
                env->getParser()->invoke(env, func, val, val_func);
                if (val_func->isFunction())
                {
                    func_info = val_func->getFunction();
                    func_scope = val_func->getFunctionScope();
                    val_this.setValue(val);
                }
            }
        
        
            if (func_info == NULL)
            {
                delete val_func;
                env->setRuntimeError(rterrorTermNotFunction, NULL, m_call_text);
                return evalFailed;
            }
        }
    }
     else
    {
        env->m_valreturn_this = NULL;
        func_info = val->getFunction();
        func_scope = val->getFunctionScope();
    }


    if (func_scope == envUseGlobalScope)
        func_scope = env->getParser()->getGlobalScope();
    else if (func_scope == envUseCallerScope)
        func_scope = env;
    
    retval->reset();
    

    // set up a new environment for the function call
    ExprEnv* func_env = env->getParser()->createEnv();
    func_env->setParent(func_scope);
    func_env->m_parser_env = env->getParserContext();
    func_env->m_callee = func_info;

    // If the function is being called inside the new operator, this
    // will set |this| to the new object.  Otherwise, in the case of abc.func(),
    // |this| will be set to |abc| as in abc.func()
    if (env->m_newobj)
    {
        func_env->setThis(env->m_newobj);
        
        // for objects bound to C++ code, the prototype is
        // transfered inside kscript_object_constructor, so we
        // don't need to handle this here.  For all other objects, we
        // need to initialize the object and transfer the prototype
        // manually
        
        if (func_info->m_func_ptr != kscript_object_constructor)
        {
            env->m_newobj->setObject(Object::createObject(env));
            
            // transfer 'prototype' of the constructor to
            // the prototype of the object
            if (val)
            {
                Value* proto = val->getObject()->lookupMember(L"prototype", false);
                if (proto)
                    env->m_newobj->setPrototype(proto);
            }
        }

        env->m_newobj = NULL;
    }
     else if (val_this.isRef())
    {
        func_env->setThis(val_this.getRef());
    }
     else if (val_this.m_type != 255)
    {
        func_env->setThis(&val_this);
    }
     else
    {
        //func_env->setThis(env->getThis());
    }

    
    int res = evalSucceeded;
    size_t i;
    
    
    func_env->reserveParams(m_param_count);
    func_env->m_param_count = m_param_count;
    
    for (i = 0; i < m_param_count; ++i)
    {
        func_env->m_params[i] = m_params[i];
        
        // evaluate the params if they are not deferred
        if (!func_info->m_defer_calc)
        {    
            if (failed(m_params[i]->eval(env, func_env->m_eval_params[i])))
            {
                func_env->unref();
                delete val_func;
                return evalFailed;
            }       
        }
    }  
          
    if (func_info->m_func_ptr)
    {
        // bound function

        func_env->m_binding_param = func_info->m_binding_param;
        func_info->m_func_ptr(func_env, func_env->m_binding_param, retval);
        
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
     else if (func_info->m_sequence)
    {
        // JS function

        size_t count = func_info->m_param_ids.size();
        for (i = 0; i < count; i++)
        {
            if (i < m_param_count)
            {
                if (failed(func_env->m_eval_params[i]->eval(env, func_env->declareVar(func_info->m_param_ids[i]))))
                {
                    func_env->unref();
                    delete val_func;
                    return evalFailed;
                }
            }
             else
            {
                func_env->declareVar(func_info->m_param_ids[i])->setUndefined();
            }
        }
        
        res = func_info->m_sequence->eval(func_env, retval);
        if (res != evalSucceededReturn)
            retval->reset();
            
        if (succeeded(res))
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


    if (func_env->getRefCount() > 1)
        func_env->resetCircular();
        
    func_env->unref();
    delete val_func;

    return res;
}

int ExprFunctionCall::getType()
{
    return m_retval_type;
}



// ExprFunctionReturn implementation

ExprFunctionReturn::ExprFunctionReturn()
{
    m_expr = NULL;
}

ExprFunctionReturn::~ExprFunctionReturn()
{
    delete m_expr;
}

int ExprFunctionReturn::eval(ExprEnv* env, Value* retval)
{
    if (!m_expr)
    {
        retval->setNull();
        return evalSucceededReturn;
    }

    if (failed(m_expr->eval(env, retval)))
        return evalFailed;

    return evalSucceededReturn;
}

int ExprFunctionReturn::getType()
{
    if (!m_expr)
        return Value::typeUndefined;
    return m_expr->getType();
}



// ExprBreak implementation

ExprBreak::ExprBreak()
{
    m_label_id = 0;
}

ExprBreak::~ExprBreak()
{
}

int ExprBreak::eval(ExprEnv* env, Value* retval)
{
    env->setBreakContinueLabel(m_label_id);
    return evalSucceededBreak;
}

int ExprBreak::getType()
{
    return Value::typeNull;
}




// ExprContinue implementation

ExprContinue::ExprContinue()
{
    m_label_id = 0;
}

ExprContinue::~ExprContinue()
{
}

int ExprContinue::eval(ExprEnv* env, Value* retval)
{
    env->setBreakContinueLabel(m_label_id);
    return evalSucceededContinue;
}

int ExprContinue::getType()
{
    return Value::typeNull;
}



// ExprTryCatch implementation

ExprTryCatch::ExprTryCatch()
{
    m_try_block = NULL;
    m_catch_block = NULL;
    m_finally_block = NULL;
    m_catch_param = 0;
}

ExprTryCatch::~ExprTryCatch()
{
    delete m_try_block;
    delete m_catch_block;
    delete m_finally_block;
}


int ExprTryCatch::eval(ExprEnv* env, Value* retval)
{
    int res = evalSucceeded;
    
    if (m_try_block)
    {
        res = m_try_block->eval(env, retval);
        
        if (succeeded(res) && res != evalSucceeded)
            return res;
    }
    
    if (env->getRuntimeError() != rterrorNone)
    {
        if (env->getRuntimeError() == rterrorExit)
        {
            return evalFailed;
        }

        if (m_catch_block)
        {
            // error happened, run the catch block
            env->setRuntimeError(rterrorNone);
            
            ExprEnv* catch_env = env->m_parser->createEnv();
            
            catch_env->setParent(env);
            catch_env->m_val_this = env->m_val_this;
            catch_env->m_valreturn_this = env->m_valreturn_this;
            catch_env->m_valreturn_arrinfo = env->m_valreturn_arrinfo;
            catch_env->reserveParams(0);
            catch_env->declareVar(m_catch_param)->setValue(
                                           env->m_parser->m_exception_obj);

            res = m_catch_block->eval(catch_env, retval);
            
            catch_env->unref();
            
            if (env->getRuntimeError() != rterrorNone)
                return evalFailed;
                
            if (succeeded(res) && res != evalSucceeded)
                return res;
        }
    }
    
    if (m_finally_block)
    {
        int old_error = env->getRuntimeError();
        env->setRuntimeError(rterrorNone);
        res = m_finally_block->eval(env, retval);
        if (env->getRuntimeError() != rterrorNone)
            return evalFailed;
        env->setRuntimeError(old_error);
        if (old_error != rterrorNone)
            return evalFailed;
        if (succeeded(res) && res != evalSucceeded)
            return res;
    }
    
    return evalSucceeded;
}

int ExprTryCatch::getType()
{
    if (m_finally_block)
        return m_finally_block->getType();
    if (m_try_block)
        return m_try_block->getType();
    return Value::typeUndefined;
}


// ExprThrow implementation

ExprThrow::ExprThrow()
{
    m_expr = NULL;
}

ExprThrow::~ExprThrow()
{
    delete m_expr;
}

int ExprThrow::eval(ExprEnv* env, Value* retval)
{
    Value v;
    if (!succeeded(m_expr->eval(env, &v)))
        return evalFailed;
    env->setRuntimeError(rterrorThrownException, &v);
    return evalFailed;
}

int ExprThrow::getType()
{
    return Value::typeUndefined;
}




// ExprArguments implementation

ExprArguments::ExprArguments()
{
}

ExprArguments::~ExprArguments()
{
}

int ExprArguments::eval(ExprEnv* env, Value* retval)
{
    // find the right environment
    while (!env->m_callee)
    {
        env = env->m_parent;
        if (!env)
        {
            retval->setNull();
            return evalSucceeded;
        }
    }
    


    if (env->m_arguments)
    {
        retval->setValue(env->m_arguments);
        return evalSucceeded;
    }
    
    env->m_arguments = new Value;
    env->m_arguments->setObject(Object::createObject(env));
    
    size_t i;
    for (i = 0; i < env->m_param_count; ++i)
        env->m_arguments->setMemberI((int)i, env->m_eval_params[i]);


    Value* vlength = env->m_arguments->getMember(L"length");
    vlength->setInteger((int)env->m_param_count);
    vlength->setAttributes(Value::attrDontEnum);
    
    Value* vfunc = env->m_arguments->getMember(L"callee");
    vfunc->setFunction(env->m_callee);
    vfunc->setAttributes(Value::attrDontEnum);
    
    retval->setValue(env->m_arguments);
    
    return evalSucceeded;
}

int ExprArguments::getType()
{
    return Value::typeObject;
}



// ExprSequence implementation

ExprSequence::ExprSequence()
{
    m_penv = NULL;
    m_create_env = true;
    m_always_wants_return = false;
    m_instructions_arr = NULL;
    m_instructions_count = 0;
    m_func_count = 0;
    m_var_count = 0;
    m_label_id = 0;
}

ExprSequence::~ExprSequence()
{
    std::vector<ExprElement*>::const_iterator it, it_end = m_instructions.end();

    for (it = m_instructions.begin(); it != it_end; ++it)
        delete (*it);
    
    delete[] m_instructions_arr;
}


void ExprSequence::addFunctionElement(ExprElement* inst)
{
    m_instructions.insert(m_instructions.begin() + m_func_count, inst);
    m_func_count++;
}

void ExprSequence::addVarElement(ExprElement* inst)
{
    m_instructions.insert(m_instructions.begin() + m_func_count + m_var_count, inst);
    m_var_count++;
}

void ExprSequence::prependElement(ExprElement* inst)
{
    m_instructions.insert(m_instructions.begin() + m_func_count + m_var_count, inst);
}

void ExprSequence::appendElement(ExprElement* inst)
{
    m_instructions.push_back(inst);
}

size_t ExprSequence::getElementCount()
{
    return m_instructions.size();
}

int ExprSequence::eval(ExprEnv* env, Value* retval)
{
    size_t i;

    if (m_instructions_count != m_instructions.size())
    {
        m_instructions_count = m_instructions.size();
        
        delete[] m_instructions_arr;
        m_instructions_arr = new ExprElement*[m_instructions_count];
        
        i = 0;
        std::vector<ExprElement*>::iterator it, it_end = m_instructions.end();
        for (it = m_instructions.begin(); it != it_end; ++it)
            m_instructions_arr[i++] = *it;
    }
    
    if (m_instructions_count == 0)
    {
        retval->setUndefined();
        return evalSucceeded;
    }
    
    
    ExprEnv* seq_env;
    if (m_create_env)
    {
        seq_env = env->getParser()->createEnv();
        seq_env->setParent(env);
        seq_env->setThis(env->getThis());
        seq_env->m_parser_env = m_penv;
        seq_env->m_val_with = env->m_val_with;
    }
     else
    {
        seq_env = env;
        seq_env->m_parser_env = m_penv;
    }
    
    
    int res = evalSucceeded;

    for (i = env->m_switchcase_offset; i < m_instructions_count; ++i)
    {
        // execute the instruction
        res = m_instructions_arr[i]->eval(seq_env, retval);

        if (res == evalSucceededReturn)
        {
            // we're done processing the sequence, but leave
            // the return result for the caller to process
            break;
        }
        
        if (m_always_wants_return && succeeded(res))
        {
            if (i+1 >= m_instructions_count)
                break;
        }
        
        // clear out any existing object from the return result,  
        // in case retval is not set by the any further instructions
        if (retval->hasObject())
        {
            retval->releaseObject();
        }
        
        // if 'break' or 'continue' was called in a loop, exit the sequence
        if (res == evalSucceededBreak || res == evalSucceededContinue)
        {
            int label_id = env->getBreakContinueLabel();
            if (label_id != 0 && label_id == m_label_id)
            {
                // stop propagation
                res = evalSucceeded;
            }
            break;
        }
        
        if (failed(res))
            break;

        if (env->isCancelled())
        {
            res = evalFailedCancelled;
            break;
        }
    }

    if (m_create_env)
        seq_env->unref();

    return res;
}

int ExprSequence::getType()
{
    if (m_instructions.size() == 0)
        return Value::typeUndefined;

    return (*(m_instructions.rbegin()))->getType();
}




// ExprParserEnv implementation

ExprParserEnv::ExprParserEnv(ExprParserEnv* parent)
{
    m_parent = parent;
    m_parser = NULL;
    m_seq = NULL;
    m_func = NULL;
    m_super_called = false;
    m_eval = false;

    if (m_parent)
    {
        m_parser = m_parent->m_parser;
        m_parent->addChild(this);
    }
}

ExprParserEnv::~ExprParserEnv()
{
    std::vector<ExprParserEnv*>::iterator it, it_end = m_children.end();
    for (it = m_children.begin(); it != it_end; ++it)
        delete (*it);
        
    std::map<std::wstring, ExprParserToken*>::iterator tk_it;
    for (tk_it = m_tokens.begin(); tk_it != m_tokens.end(); ++tk_it)
        delete tk_it->second;
    for (tk_it = m_itokens.begin(); tk_it != m_itokens.end(); ++tk_it)
        delete tk_it->second;
}

void ExprParserEnv::addChild(ExprParserEnv* child)
{
    m_children.push_back(child);
}

void ExprParserEnv::removeChild(ExprParserEnv* child)
{
    std::vector<ExprParserEnv*>::iterator it, it_end = m_children.end();
    for (it = m_children.begin(); it != it_end; ++it)
    {
        if (*it == child)
        {
            m_children.erase(it);
            return;
        }
    }
}

void ExprParserEnv::reserve(size_t amount)
{
}

int ExprParserEnv::declareToken(const std::wstring& _token,
                                bool case_sense,
                                ExprVarBinding* binding)
{
    std::map<std::wstring, ExprParserToken*>::iterator it;
    std::wstring token = xtrim(_token);
         
    // first, look up the token to see if it already
    // exists in this scope
    if (case_sense)
    {
        it = m_tokens.find(token);
        if (it != m_tokens.end())
            return it->second->id;
    }
     else
    {
        makeLower(token);
        it = m_itokens.find(token);
        if (it != m_itokens.end())
            return it->second->id;
    }

    ExprParserToken* t = new ExprParserToken;
    t->name = token;
    t->case_sense = case_sense;
    t->binding = binding;
    t->id = m_parser->getSymbolId(token);
    if (case_sense)
        m_tokens[token] = t;
         else
        m_itokens[token] = t;

    return t->id;
}

ExprParserToken* ExprParserEnv::lookupToken(const std::wstring& _token)
{
    std::wstring token = xtrim(_token);
    return _lookupToken(token);
}

ExprParserToken* ExprParserEnv::_lookupToken(const std::wstring& token)
{
    std::map<std::wstring, ExprParserToken*>::iterator it;
    it = m_tokens.find(token);
    if (it != m_tokens.end())
    {
        // token found, return it
        return it->second;
    }
    

    // if necessary, look up the token in the case insensitive map
    if (m_itokens.size() > 0)
    {
        std::wstring itoken = token;
        makeLower(itoken);
        it = m_itokens.find(itoken);
        if (it != m_itokens.end())
        {
            // token found, return it
            return it->second;
        }
    }

    if (m_parent == NULL)
        return NULL;

    return m_parent->lookupToken(token);
}


bool ExprParserEnv::getTokenExistInScope(const std::wstring& _token)
{
    std::wstring token = xtrim(_token);
    std::map<std::wstring, ExprParserToken*>::iterator it;
    it = m_tokens.find(token);
    if (it != m_tokens.end())
        return true;
    
    // if necessary, look up the token in the case insensitive map
    if (m_itokens.size() > 0)
    {
        std::wstring itoken = token;
        makeLower(itoken);
        it = m_itokens.find(itoken);
        if (it != m_itokens.end())
            return true;
    }

    return false;
}

bool ExprParserEnv::getTokenExist(const std::wstring& token)
{
    return (lookupToken(token) != NULL);
}

ExprParserToken* ExprParserEnv::getToken(const std::wstring& token)
{
    ExprParserToken* t = lookupToken(token);

    if (t)
    {
        return t;
    }

    declareToken(token, true);

    return lookupToken(token);
}




ExprParserEnv* ExprParserEnv::getHighestContext()
{
    // getHighestContext() gets the highest parser
    // environment with a sequence
    
    ExprParserEnv* penv = this;
    ExprParserEnv* ret = NULL;

    ret = this;

    while (penv->m_parent)
    {
        penv = penv->m_parent;

        if (penv->m_seq)
            ret = penv;
    }

    return ret;
}


ExprParserEnv* ExprParserEnv::getVarContext()
{
    if (!m_parent)
        return this;
        
    ExprParserEnv* penv = this;
    ExprSequence* seq = m_seq;
    ExprSequence* func_seq = NULL;
    
    while (1)
    {
        if (penv->m_func)
        {
            func_seq = seq;
            break;
        }

        if (!penv->m_parent)
            break;
            
        penv = penv->m_parent;
        if (penv->m_seq)
            seq = m_seq;
    }
    
    if (func_seq)
        return func_seq->m_penv;

    return getHighestContext();
}

ExprParserEnv* ExprParserEnv::getEvalContext()
{
    ExprParserEnv* penv = this;
    
    while (penv)
    {
        if (penv->m_eval)
            return penv;
        penv = penv->m_parent;
    }
    
    return NULL;
}
    
ExprSequence* ExprParserEnv::getSequence()
{
    ExprParserEnv* penv = this;

    while (penv)
    {
        if (penv->m_seq)
            return penv->m_seq;
        
        penv = penv->m_parent;
    }
    
    return NULL;
}


ExprClassInfo* ExprParserEnv::getClassInfo()
{
    std::wstring class_name = getClass();
    if (class_name.empty())
        return NULL;

    return m_parser->getClassInfo(class_name);
}

void ExprParserEnv::setClass(const std::wstring& class_name)
{
    m_class = class_name;
}

std::wstring ExprParserEnv::getClass()
{
    if (!m_class.empty())
        return m_class;

    if (!m_parent)
        return L"";

    return m_parent->getClass();
}

bool ExprParserEnv::getSuperCalled()
{
    if (m_super_called)
        return true;

    std::vector<ExprParserEnv*>::iterator it, it_end = m_children.end();
    for (it = m_children.begin(); it != it_end; ++it)
    {
        if ((*it)->getSuperCalled())
            return true;
    }

    return false;
}


// ExprClassInfo implementation

ExprClassInfo::ExprClassInfo(ExprParser* parser)
{
    m_parser = parser;
    m_cstdef = NULL;
    m_base = NULL;
    m_objinit_func = NULL;
    m_class_obj = NULL;
    
    if (parser->m_classes.empty())
        m_prototype.setObject(new ValueObject);
         else
        m_prototype.setObject(Object::createObject(parser));
}

ExprClassInfo::~ExprClassInfo()
{
    std::map<std::wstring,ExprElement*>::iterator it;
    for (it = m_members.begin(); it != m_members.end(); ++it)
        delete it->second;
}


void ExprClassInfo::addMember(const std::wstring& name, ExprElement* expr)
{
    m_members[name] = expr;
}

void ExprClassInfo::addFunction(const std::wstring& name, ExprBindFunc func, void* param)
{
    Function* funcdef = m_parser->createFunction();
    funcdef->m_func_ptr = func;
    funcdef->m_binding_param = param;

    Value* val = new Value;
    val->setFunction(funcdef);
    
    m_members[name] = val;
    
    if (name == L"constructor" || name == m_name)
    {
        // constructor method found
        m_cstdef = funcdef;
        
        if (m_name != L"Object")
        {
            // make sure the prototype hooks up to the object prototype
            // this applies for all classes except Object itself
            m_prototype.setPrototype(&m_parser->m_object_prototype);
        }
        
        Function* def = m_parser->createFunction(&m_prototype);
        def->m_func_ptr = (ExprBindFunc)kscript_object_constructor;
        def->m_binding_param = this;
        Value* val_prototype = def->getMember(L"prototype");
        val_prototype->setAttributes(Value::attrDontEnum | Value::attrDontDelete | Value::attrReadOnly);
        
        m_class_obj->setFunction(def);
    }
    

    m_class_obj->getMember(name)->setValue(val);
}

void ExprClassInfo::addPrototypeMember(const std::wstring& name, Value* val)
{
    m_prototype.createMember(name, 0)->setValue(val);
}

void ExprClassInfo::addPrototypeFunction(const std::wstring& name, ExprBindFunc func, void* param)
{
    Function* funcdef = m_parser->createFunction();
    funcdef->m_func_ptr = func;
    funcdef->m_binding_param = param;

    m_prototype.createMember(name, 0)->setFunction(funcdef);
}

Value* ExprClassInfo::addStaticMember(const std::wstring& name, Value* val)
{
    Value* v = m_class_obj->getMember(name);
    v->setValue(val);
    return v;
}

void ExprClassInfo::addStaticFunction(const std::wstring& name, ExprBindFunc func, void* param)
{
    Function* funcdef = m_parser->createFunction();
    funcdef->m_func_ptr = func;
    funcdef->m_binding_param = param;

    Value val;
    val.setFunction(funcdef);
    
    m_class_obj->getMember(name)->setValue(&val);
}

bool ExprClassInfo::getMemberExistsInHierarchy(const std::wstring& name)
{
    if (m_members.find(name) != m_members.end())
        return true;
    if (m_prototype.lookupMember(name, false))
        return true;   
     
    ExprClassInfo* base = getBaseClass();
    if (base)
        return base->getMemberExistsInHierarchy(name);
    return false;
}

bool ExprClassInfo::getMemberExists(const std::wstring& name)
{
    if (m_members.find(name) != m_members.end())
        return true;
    return false;
}

Value* ExprClassInfo::getPrototypeMember(const std::wstring& name)
{
    return m_prototype.getMember(name);
}

ExprClassInfo* ExprClassInfo::getBaseClass()
{
    if (!m_base)
    {
        if (m_base_name.length() > 0)
            m_base = m_parser->getClassInfo(m_base_name);
    }
    
    return m_base;
}

void ExprClassInfo::setupObject(ValueObject* obj)
{
    if (!m_base)
    {
        if (m_base_name.length() > 0)
            m_base = m_parser->getClassInfo(m_base_name);
    }

    // initialize object members from the base class(es)
    if (m_base)
        m_base->setupObject(obj);
    
    obj->setPrototype(&m_prototype);
    
    // initialize object members
    if (!m_members.empty())
    {
        std::map<std::wstring, ExprElement*>::const_iterator it, it_end;
        it_end = m_members.end();
        for (it = m_members.begin(); it != it_end; ++it)
        {
            // create a non-enumerable member
            Value* member = obj->createMember(it->first, Value::attrDontEnum);
            
            if (it->second)
                it->second->eval(m_parser->getGlobalScope(), member);
                 else
                member->setNull();
        }
    }
}



int ExprParser::getSymbolId(const std::wstring& symbol)
{
    std::map<std::wstring, int>::iterator it;
    it = m_dictionary_m.find(symbol);
    if (it == m_dictionary_m.end())
    {
        if (m_symbol_counter >= (int)m_dictionary_v.size())
            m_dictionary_v.resize(m_symbol_counter + 20);
    
        m_dictionary_m[symbol] = m_symbol_counter;
        m_dictionary_v[m_symbol_counter] = symbol;
        return m_symbol_counter++;
    }
    
    return it->second;
}


const std::wstring& ExprParser::getSymbolName(int id)
{
    if (id < 0 || (size_t)id >= m_dictionary_v.size())
        return g_empty_string;
    
    return m_dictionary_v[id];
}


bool ExprParser::parseParams(ExprParserEnv* penv,
                             wchar_t* params,
                             std::vector<ExprElement*>& pparam,
                             wchar_t** endloc)
{
    pparam.clear();

    while (iswspace(*params))
        params++;

    if (*params == '(')
        params++;

    wchar_t* next_param = params;
    wchar_t* param_text;
    ExprElement* param;
    bool error = false;
    bool res;

    // parse the parameters, if any
    while (1)
    {
        wchar_t* param_end;
        wchar_t temp_paramc;

        res = zl_getParam(&next_param,
                          &param_text,
                          &param_end,
                          L"([{",
                          L")]}");

        if (!param_text)
        {
            // we are returned this when there is nothing left for us:
            // this is ok only with 0-parameter functions, i.e. if we
            // already have parameters, this in an invalid parse

            if (pparam.size() > 0)
            {
                error = true;
                if (endloc)
                    *endloc = params;
                m_error_loc = params;
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
            }
            
            break;
        }
        
        if (*param_text == ',')
        {
            if (endloc)
                *endloc = param_text;
            m_error_loc = param_text;
            m_error_code = errorMalformedStatement;
            m_error_text = L"";
            
            error = true;
            break;
        }

        temp_paramc = *param_end;
        *param_end = 0;
        param = parseElement(penv, param_text, endloc);
        *param_end = temp_paramc;

        if (param)
        {
            pparam.push_back(param);
        }
         else
        {
            error = true;
            break;
        }

        if (!res)
            break;
    }


    // do some extra error checking

    if (!error)
    {    
        // at this point, we need to find a close parenthesis.
        // If we didn't, we have an error
        if (*next_param != ')')
        {
            if (endloc)
                *endloc = next_param;
                
            m_error_loc = next_param+1;
            m_error_code = errorMissingCloseParenthesis;
            m_error_text = L"";

            error = true;
        }
         else
        {
            // make sure there are no unwanted tokens after the last parenthesis
            next_param++;

            while (iswspace(*next_param))
                next_param++;

            if (*next_param)
            {
                if (endloc)
                    *endloc = next_param;

                m_error_loc = next_param;
                m_error_code = errorMissingSemicolon;
                m_error_text = L"";

                error = true;
            }
        }
        
        if (!error)
        {
            if (endloc)
                *endloc = next_param;
        }
    }


    if (error)
    {
        // deallocate all the work we had done
        std::vector<ExprElement*>::iterator it;
        for (it = pparam.begin(); it != pparam.end(); ++it)
            delete (*it);

        pparam.clear();
        return false;
    }

    return true;
}




inline bool isToken(wchar_t* ch, const wchar_t* token)
{
    while (iswspace(*ch))
        ch++;
    while (*token)
    {
        if (*ch != *token)
            return false;
        ch++;
        token++;
    }
    if (!*ch || iswspace(*ch))
        return true;
    return false;
}


inline bool isTokenNoCase(wchar_t* ch, const wchar_t* token)
{
    while (iswspace(*ch))
        ch++;
    while (*token)
    {
        if (towupper(*ch) != towupper(*token))
            return false;
        ch++;
        token++;
    }
    if (!*ch || iswspace(*ch))
        return true;
    return false;
}


int getReturnValType(const std::wstring& format,
                     ExprElement* elements[],
                     int element_count);


wchar_t* ExprParser::getNextOper(wchar_t* expr,
                                 wchar_t* expr_end,
                                 ExprOperator** entry)
{
    ExprOperator* last_entry = NULL;
    wchar_t* last_ptr = NULL;
    wchar_t* p;
    wchar_t* letter;
    wchar_t* border = NULL;
    wchar_t first_oper_char;
    
    while (isWhiteSpaceOrLS(*expr))
        expr++;
        
        
    // in instances of one character, no operators possible
    if (expr_end - expr < 2)
        return NULL;

    wchar_t* end = expr_end;

         
    std::vector<ExprOperator>::iterator it, it_end = m_opers.end();

    for (it = m_opers.begin(); it != it_end; ++it)
    {
        end = expr_end;
        
        while (1)
        {
            p = zl_stristr(expr,
                           end,
                           it->m_name,
                           it->m_alpha,
                           it->m_case_sense,
                           (it->m_direction == ExprOperator::directionLeftToRight) ? true : false);
            if (!p)
                break;
            
            end = p;
            
            first_oper_char = it->m_name[0];
            if (first_oper_char == '.')
            {
                // help out the parsing of the array operator
                // (it should ignore numbers)

                bool is_number = true;
                letter = p-1;
                while (letter >= expr)
                {
                    if (!iswdigit(*letter))
                    {
                        is_number = false;
                        break;
                    }
                    letter--;
                }
                
                if (is_number)
                    continue;
            }
             else if (first_oper_char == '+' || first_oper_char == '-')
            {
                // help out the parsing of the  + and -, which should
                // ignore constants represented with exponents
                // (it should ignore numbers)

                if (p >= expr+2 && towupper(*(p-1)) == L'E' && iswdigit(*(p-2)))
                    continue;
            }

            
            
            if (first_oper_char != L'.')
            {
                if (it->m_type != ExprOperator::typeUnary)
                {
                    // if the test operator is not unary:
                    //     1) there must not be another operator directly before it and
                    //     2) there must be some kind of term before it

                    if (p == expr || p+1 >= expr_end)
                    {
                        // we are at the beginning or end , so the operator
                        // would have to be unary to succeed. The test operator
                        // is not, so skip it.
                        continue;
                    }
                    

                    bool valid = true;
                    wchar_t* ch = p-1;
                    while (ch >= expr)
                    {
                        if (ch == border)
                        {
                            if (last_entry->m_type == ExprOperator::typeUnary)
                            {
                                // we ran into an operator (probably a unary) directly
                                // before this one, so this can't be the correct one
                                if (*border != '+' && *border != '-')
                                {
                                    valid = false;
                                    break;
                                }
                            }
                            
                        }
                            
                        if (iswalpha(*ch) || iswdigit(*ch) || wcschr(L"\"'()[]{}$_", *ch))
                        {
                            // some identifier or term, so operator is ok
                            break;
                        }

                        if (ch == expr)
                        {
                            // we reached the beginning, so this is not
                            // the correct operator, skip it
                            valid = false;
                            break;
                        }
                        
                        if (wcschr(L"!@#%^&*=|,<>/:", *ch))
                        {
                            // some other operator, which means this would
                            // have to be a unary operator, which it isn't
                            valid = false;
                            break;
                        }

                        ch--;
                    }
                    if (!valid)
                        continue;
                }

                // check letter before operator
                letter = p-1;
                if (letter >= expr &&
                    !(iswspace(*letter) ||
                      iswalpha(*letter) ||
                      iswdigit(*letter) ||
                      wcschr(L"/$\"'()[]{}-+_.", *letter)))
                {
                    continue;
                }
                
                // check letter after operator
                letter = p+it->m_name.length();
                if (!(iswspace(*letter) ||
                      iswalpha(*letter) ||
                      iswdigit(*letter) ||
                      wcschr(L"/$\"'()[]{}-+_~!*.,", *letter)))
                {
                    continue;
                }
                
                // prevent confusion between ++/--,  unary +/-, and normal +/-
                if (it->m_name == L"+" || it->m_name == L"-")
                {
                    if (it->m_type == ExprOperator::typeUnary)
                    {
                        if (*letter == first_oper_char ||
                            ((p-1) >= expr && *(p-1) == first_oper_char))
                        {
                            continue;
                        }
                        
                        // unary plus and minus must come first
                        if (!*(p+1))
                            continue;
                    }
                     else
                    {
                        // if + or - occurs at the very beginning, this is unary, not regular +/-
                        if (p-1 < expr)
                            continue;
                        
                        // if the letter after the operator is a + or a -, this means
                        // that we are standing on a ++ or a --, so skip it
                        if (*letter == first_oper_char)
                            continue;
                        
                        letter = p-1;
                        while (letter >= expr && iswspace(*letter))
                            letter--;
                        size_t c = 0;
                        while (letter >= expr && (*letter == '+' || *letter == '-'))
                        {
                            c++;
                            letter--;
                        }
                        
                        if (c == 1 || c == 3) 
                            continue;
                    }
                }

                if (it->m_name == L"++" || it->m_name == L"--")
                {
                    bool ok = true;
                    
                    // these need to check for carriage returns between
                    // the operator and the operand
                    if (p == expr)
                    {
                        // prefix operator, look after it
                        wchar_t* ch = p+2;
                        while (ch < expr_end && *ch && iswspace(*ch))
                        {
                            if (isLineSeparator(*ch))
                            {
                                ok = false;
                                break;
                            }
                            ++ch;
                        }
                    }
                     else
                    {
                        // postfix operator, look before it
                        wchar_t* ch = p-1;
                        while (ch >= expr && *ch && iswspace(*ch))
                        {
                            if (isLineSeparator(*ch))
                            {
                                ok = false;
                                break;
                            }
                            --ch;
                        }
                    }
                   
                    if (!ok)
                        continue;
                }

            }
                
            
            break;
        }
        
        if (!p)
            continue;
        
        

        if (last_entry)
        {
            if (it->m_priority > last_entry->m_priority)
            {
                continue;
            }
            if (it->m_priority == last_entry->m_priority &&
                it->m_direction == ExprOperator::directionRightToLeft &&
                p <= last_ptr)
            {
                continue;
            }
            if (it->m_priority == last_entry->m_priority &&
                it->m_direction == ExprOperator::directionLeftToRight &&
                p >= last_ptr)
            {
                continue;
            }
        }
        
        last_ptr = p;
        last_entry = &(*it);
        border = p + it->m_name.length() - 1;
        if (entry)
        {
            *entry = &(*it);
        }
    }

    return last_ptr;
}


ExprElement* ExprParser::parseStringLiteral(ExprParserEnv* penv,
                                            wchar_t* expr,
                                            wchar_t** endloc,
                                            size_t prealloc_size)
{
    static const wchar_t* hexchars = L"0123456789ABCDEF";
    static const wchar_t* octchars = L"01234567";
    
    wchar_t quote_char = *expr;
    if (quote_char != '"' && quote_char != '\'')
        return NULL;
        
    if (!prealloc_size)
    {
        // figure out how much space we will need for the parse
        // for now, we can just search for the end of the string;
        // it'd be better to search for the close quote
        wchar_t* end = expr+wcslen(expr);
        prealloc_size = end-expr;
    }

    Value* v = new Value;
    v->allocString((int)prealloc_size+5);
    
    wchar_t* start = v->getString();
    wchar_t* src = expr+1;
    wchar_t* dest = start;
    while (*src)
    {
        if (m_language == optionLanguageECMAScript &&
            *src == '\\')
        {
            src++;

            // handle backslash escape character
            switch (*src)
            {
                case L'b':   *dest = '\b'; break;
                case L'f':   *dest = '\f'; break;
                case L'n':   *dest = '\n'; break;
                case L'r':   *dest = '\r'; break;
                case L't':   *dest = '\t'; break;
                case L'v':   *dest = '\v'; break;
                case L'"':   *dest = '\"'; break;
                case L'\'':  *dest = '\''; break;
                case L'\\':  *dest = '\\'; break;
                case L'0':
                {
                    if (!*(src+1) || !iswdigit(*(src+1)))
                    {
                        *dest = '\0';
                        break;
                    }
                     else
                    {
                        // fall through to octal processing
                    }
                }
                
                case L'1': case L'2':
                case L'3': case L'4':
                case L'5': case L'6': case L'7':
                {
                    wchar_t* p = src;
                    wchar_t ch = 0;
                    
                    while (*p >= '0' && *p <= '7')
                    {
                        if ((ch*8) + (*p-'0') > 255)
                            break;
                        ch = (ch*8) + (*p-'0');
                        src++;
                        ++p;
                    }
                    
                    src--;
                    *dest = ch;
                }
                break;
                
                
                case L'x':
                {
                    wchar_t ch = (wchar_t)-1;
                    
                    // hex escape sequence
                    if (*(src+1) && *(src+2))
                    {
                        const wchar_t* res;
                        wchar_t d1, d2;
                        
                        res = wcschr(hexchars, ::towupper(*(src+1)));
                        if (res)
                        {
                            d1 = (wchar_t)(res - hexchars);
                            res = wcschr(hexchars, ::towupper(*(src+2)));
                            if (res)
                            {
                                d2 = (wchar_t)(res - hexchars);
                                ch = (d1*16)+d2;
                            }
                        }
                    }
                    
                    if (ch == (wchar_t)-1)
                    {
                        *dest = *src;
                    }
                     else
                    {
                        *dest = ch;
                        src += 2;
                    }
                }
                break;
                
                case L'u':
                {
                    // unicode escape sequences
                    
                    if (*(src+1) && *(src+2) && *(src+3) && *(src+4))
                    {
                        wchar_t* s = src+1;
                        const wchar_t* res;
                        unsigned int i, len = 4, mul = 4096, ch = 0;

                        for (i = 0; i < len; ++i)
                        {
                            res = wcschr(hexchars, ::towupper(*(s+i)));
                            if (!res)
                                break;
                            ch += (wchar_t)((res-hexchars)*mul);
                            mul /= 16;
                        }

                        if (i == len)
                        {
                            *dest = (wchar_t)ch;
                            src += 4;
                        }
                         else
                        {
                            *dest = *src;
                        }
                    }
                     else
                    {
                        *dest = *src;
                    }
                }
                break;
                
                default:     *dest = *src; break;
            }

            dest++;
            src++;
            continue;
        }


        if (*src == quote_char)
        {
            // handle double quote in non-javascript parsers
            if (m_language == optionLanguageECMAScript || *(src+1) != quote_char)
                break;
            src++;
        }

        *dest = *src;
        dest++;
        src++;
    }
    
    *dest = 0;
    
    if (!*src)
    {
        delete v;
        if (endloc)
            *endloc = src;
        return NULL;
    }


    // check after the string content for unwanted tokens
    src++;
   
    if (endloc)
        *endloc = src;
    v->setDataLen((int)(dest-start));
    return static_cast<ExprElement*>(v);
}


ExprElement* ExprParser::parseElement(ExprParserEnv* penv,
                                      wchar_t* expr,
                                      wchar_t** endloc)
{
#ifdef _DEBUG
    // allow test suite to work in debug mode (which chews up way more stack space during recursion)
    ExprDepthCounter cnt(&m_current_depth);
    if (m_current_depth >= 220)
    {
        if (endloc)
            *endloc = expr;
        
        m_error_loc = expr;
        m_error_code = errorUnexpectedToken;
        m_error_text = L"";

        return NULL;
    }
#endif

    while (iswspace(*expr))
        expr++;

    if (!*expr)
    {
        if (endloc)
            *endloc = expr;
        
        // empty statement
        m_error_loc = expr;
        m_error_code = errorMalformedStatement;
        m_error_text = L"";

        return NULL;
    }

    wchar_t* ch;
    wchar_t tempch;

    // find end of string
    wchar_t* end = expr+wcslen(expr);
    
    if (endloc)
        *endloc = end;
    
    end--;
    ch = expr;
    while (end >= ch && iswspace(*end))
        end--;
    
    if (end < ch)
    {
        if (endloc)
            *endloc = expr;
        m_error_loc = expr; // end
        m_error_code = errorMalformedStatement;
        m_error_text = L"";
        return NULL;
    }
    


    // check to see if we are declaring a variable
    //if (m_language == optionLanguageECMAScript)
    //{
    //    if (!wcsncmp(ch, L"function", 8) && !iswalpha(*(ch+8)))
    //        return parseFunction(penv, expr, endloc, NULL, true /* function expr */);
    //}


    // check for an operator
    wchar_t* oper = NULL;
    ExprOperator* entry = NULL;

    while (1)
    {        
        while (iswspace(*ch))
            ch++;

        // if the string length is zero, there are no operators
        if (end-ch == 0)
            break;
        
        // look for operator
        oper = getNextOper(ch, end+1, &entry);

        if (!oper)
        {
            // get rid of superfluous parenthesis

            while (iswspace(*ch))
                ch++;
            while (end >= ch && iswspace(*end))
                end--;
                
            if (end < ch)
            {
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }

            if (*ch == L'(' && *end == L')')
            {
                wchar_t* exprend = NULL;
                
                *end = 0;
                ExprElement* e = parseElement(penv, ch+1, &exprend);
                *end = ')';
                
                skipWhiteSpaceOrLS(exprend);
                if (exprend && exprend != end)
                {
                    delete e;
                    m_error_loc = end; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";
                    return NULL;
                }
                
                if (e)
                {
                    if (endloc)
                        *endloc = end+1;
                }
                
                return e;
            }
        }

        break;
    }

    tempch = *(end+1);
    *(end+1) = 0;
    SetCharOnExitScope sc;
    sc.ptr = end+1;
    sc.ch = tempch;


    if (m_language == optionLanguageECMAScript)
    {
        if (!oper && *ch == L'[')
        {
            // array literal notation         
            return parseArrayLiteral(penv, ch, endloc);
        }
         else if (!oper && *ch == L'{')
        {
            // object literal notation (json)          
            return parseObjectLiteral(penv, ch, endloc);
        }
         else if (!oper && *ch == L'/')
        {
            return parseRegexLiteral(penv, ch, endloc);
        }

        // check for 'new' here
        if (nextTokenIs(ch, L"new", 3))
        {            
            wchar_t* open_paren = wcschr(ch, L'(');
            if (open_paren == NULL || open_paren == oper)
            {
                ch += 3;
                while (isWhiteSpaceOrLS(*ch))
                    ch++;
                
                ExprElement* e = parseElement(penv, ch, endloc);
                if (!e)
                    return NULL;

                bool is_function_call = false;
                
                wchar_t* p = ch;
                while (*p)
                {
                    if (*p == '(')
                    {
                        is_function_call = true;
                        break;
                    }
                    ++p;
                }
                
                if (!is_function_call)
                {
                    // if the symbol after "new" had no parentheses,
                    // we need to package up the function call ourselves
                    ExprFunctionCall* call = new ExprFunctionCall;
                    call->m_func_ptr = e;
                    call->m_call_text = ch;
                    call->m_retval_type = Value::typeUndefined;
                    call->m_param_count = 0;
                    e = call;
                }
                

                ExprNew* funcret = new ExprNew;
                funcret->m_expr = e;
            
                return funcret;
            }
        }
        
        if (nextTokenIs(ch, L"function", 8))
        {
            bool parse_function_element = false;
            
            // if we've found a '(', that could mean a function invocation.
            // in order for this to be a function element parse, the found '('
            // needs to be the very first one
            if (oper && *oper == '(')
            {
                if (wcschr(ch, '(') == oper)
                    parse_function_element = true;
            }
     
            if (parse_function_element)
                return parseFunction(penv, ch, endloc, NULL, true /* function expr */);
        }

        // this handles ternary conditionals ("condition ? if_true : if_false")
        if (oper && *oper == L'?')
        {
            wchar_t* qm = oper;
            wchar_t* colon;
            
            // try to find the colon
            colon = zl_strchr(qm+1, L':', L"([{", L")]}");
            if (!colon)
            {
                // malformed if statement
                m_error_loc = end; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }
            
            
            ExprIf* e_if = new ExprIf;
            
            // parse the condition before the ?
            *qm = 0;
            e_if->m_condition = parseElement(penv, ch);
            *qm = L'?';
            
            if (!e_if->m_condition)
            {
                delete e_if;
                return NULL;
            }
            

            // get the first (and perhaps only) statement
            *colon = 0;
            e_if->m_iftrue = parseElement(penv, qm+1, endloc);
            *colon = L':';
            
            if (!e_if->m_iftrue)
            {
                // malformed statement
                delete e_if;
                return NULL;
            }


            e_if->m_iffalse = parseElement(penv, colon+1, endloc);
            if (!e_if->m_iffalse)
            {
                delete e_if;
                return NULL;
            }

            return e_if;
        }
    }
    

    // look for a function call

    if (oper && *oper == L'(')
    {
        wchar_t* func = oper;

        std::vector<ExprElement*> params;
        wchar_t tempc;

        // give parse hook a chance
        if (m_parse_hook_mask & ExprParseHookInfo::typeFunction)
        {
            ExprParseHookInfo info;
            info.parser = this;
            info.penv = penv;
            info.hook_param = m_parse_hook_param;
            info.expr_text = xtrim(expr);
            info.element_type = ExprParseHookInfo::typeFunction;
            info.res_element = NULL;

            bool accept;
            accept = m_parse_hook(info);

            if (accept)
            {
                return info.res_element;
            }
        }

        
        while (iswspace(*(func-1)))
            func--;

        tempc = *func;
        *func = 0x00;


        std::wstring identifier = xtrim(ch);

        ExprParserToken* token = penv->lookupToken(ch);
        ExprVarBinding* binding = NULL;

        if (token)
            binding = token->binding;

        if (0 == wcscmp(ch, L"super"))
            penv->m_super_called = true;

        // parse left of '('
        ExprElement* func_ptr = parseElement(penv, ch, endloc);
        *func = tempc;
        
        if (!func_ptr)
            return NULL;


        // first check to see if we are calling a script function
        if (!parseParams(penv, func, params, endloc))
        {
            delete func_ptr;
            return NULL;
        }

        // check out the param types to make sure type fit
        // the function binding's types
        int retval_type = Value::typeUndefined;
        if (binding && binding->m_formats.length() > 0)
        {
            ExprElement** elements = new ExprElement*[params.size()];
            for (size_t i = 0; i < params.size(); i++)
                elements[i] = params[i];

            retval_type = getReturnValType(binding->m_formats,
                                           elements,
                                           (int)params.size());

            delete[] elements;
        }

        if (retval_type == 0)
        {
            // bad retval type, so free up and bail out
            std::vector<ExprElement*>::iterator it;
            for (it = params.begin(); it != params.end(); ++it)
                delete (*it);

            // invalid parameter types
            m_error_loc = func+1;
            m_error_code = errorInvalidParameterTypes;
            m_error_text = identifier;

            delete func_ptr;
            return NULL;
        }

        ExprFunctionCall* call = new ExprFunctionCall;
        call->m_func_ptr = func_ptr;
        call->m_retval_type = retval_type;
        call->m_param_count = params.size();
        call->m_call_text = identifier;

        if (params.size() > 0)
        {
            call->m_params = new ExprElement*[params.size()];
            for (size_t i = 0; i < params.size(); i++)
                call->m_params[i] = params[i];
        }

        return call;
    }





    if (oper)
    {

        // give parse hook a chance
        if (m_parse_hook_mask & ExprParseHookInfo::typeOperator)
        {
            ExprParseHookInfo info;
            info.parser = this;
            info.penv = penv;
            info.hook_param = m_parse_hook_param;
            info.expr_text = xtrim(expr);
            info.element_type = ExprParseHookInfo::typeOperator;
            info.oper_info = entry;
            info.oper_text = entry->m_name;
            info.res_element = NULL;
            
            if (entry->m_type == ExprOperator::typeUnary)
            {
                info.oper_unary = true;
                info.oper_prefix = (oper == ch) ? true : false;

                if (info.oper_prefix)
                {
                    info.oper_left = xtrim(ch + entry->m_name.length());
                }
                 else
                {
                    wchar_t tempch = *oper;
                    *oper = 0;
                    info.oper_left = xtrim(ch);
                    *oper = tempch;
                }

            }
             else
            {
                wchar_t tempc = *oper;
                *oper = 0;
                info.oper_left = xtrim(ch);
                *oper = tempc;

                if (entry->m_type == ExprOperator::typeParenthesized)
                {
                    wchar_t* right = oper+entry->m_name.length();

                    wchar_t* pend = zl_stristr(right,
                                              NULL,
                                              entry->m_name_close,
                                              entry->m_alpha,
                                              entry->m_case_sense);

                    if (!pend)
                    {
                        // missing close operator
                        m_error_loc = right + wcslen(right);
                        m_error_code = errorMissingOperator;
                        m_error_text = entry->m_name_close;

                        return NULL;
                    }

                    tempc = *pend;
                    *pend = 0;
                    info.oper_right = xtrim(right);
                    *pend = tempc;
                }
                 else
                {
                    info.oper_right = xtrim(oper + entry->m_name.length());
                }
            }
            

            bool accept;
            accept = m_parse_hook(info);

            if (accept)
                return info.res_element;
        }



        if (*oper == '.')
        {
            SetCharOnExitScope sc(oper, '.', 0);
            
            ExprSimpleArrayLookup* e = new ExprSimpleArrayLookup;
            e->m_left = parseElement(penv, ch, endloc);
            if (!e->m_left)
            {
                delete e;
                return NULL;
            }
            
            wchar_t* right = oper+1;
            while (isWhiteSpaceOrLS(*right))
                right++;
                            
            wchar_t* p = right;
            wchar_t* end = NULL;
            
            while (*p)
            {
                if (isWhiteSpaceOrLS(*p))
                {
                    end = p;
                    while (isWhiteSpaceOrLS(*p))
                        ++p;
                    if (*p)
                    {
                        // bad character found
                        m_error_loc = p;
                        m_error_code = errorUnexpectedToken;
                        m_error_text = L"";
                        if (endloc)
                            *endloc = p;
                        delete e;
                        return NULL;
                    }
                     else
                    {
                        break;
                    }
                }
                p++;
            }
            
            end = p;
            if (endloc)
                *endloc = p;
            
            e->m_right.assign(right, end-right);
            return e;
        }
         else
        {
            // we've found an operator
            ExprInstruction* inst = new ExprInstruction;
            inst->m_func_ptr = entry->m_func_ptr;
            inst->m_defer_calc = entry->m_defer_calc;

            // check for unary operator
            if (entry->m_type == ExprOperator::typeUnary)
            {
                // allocate m_params array
                inst->m_params = new ExprElement*[1];
                inst->m_params[0] = NULL;
                inst->m_eval_params = new Value*[1];
                inst->m_eval_params[0] = new Value;

                // check for prefix or postfix
                inst->m_prefix = (oper == ch);
                inst->m_param_count = 1;


                if (inst->m_prefix)
                {
                    inst->m_params[0] = parseElement(penv, ch+entry->m_name.length(), endloc);
                }
                 else
                {
                    // check for unwanted tokens after the operator; there should be none
                    wchar_t* p = oper + entry->m_name.length();
                    while (iswspace(*p))
                        p++;
                        
                    if (*p)
                    {
                        if (endloc)
                            *endloc = p;
                        
                        m_error_loc = end+1;
                        m_error_code = errorMissingSemicolon;
                        m_error_text = L"";

                        delete inst;
                        return NULL;
                    }


                    wchar_t tempch = *oper;
                    *oper = 0;
                    inst->m_params[0] = parseElement(penv, ch, endloc);
                    *oper = tempch;
                    
                    if (endloc)
                        *endloc = oper + entry->m_name.length();
                }

                if (!inst->m_params[0])
                {
                    delete inst;
                    return NULL;
                }

                inst->m_retval_type = getReturnValType(entry->m_formats.c_str(),
                    inst->m_params, inst->m_param_count);

                return inst;
            }
             else
            {
                // allocate m_params array
                inst->m_params = new ExprElement*[2];
                inst->m_params[0] = NULL;
                inst->m_params[1] = NULL;
                inst->m_eval_params = new Value*[2];
                inst->m_eval_params[0] = new Value;
                inst->m_eval_params[1] = new Value;

                // was not a unary operator, so parse down the left and right sides
                inst->m_param_count = 2;
                wchar_t tempc = *oper;
                *oper = 0;
                inst->m_params[0] = parseElement(penv, ch, endloc);
                *oper = tempc;

                if (!inst->m_params[0])
                {
                    delete inst;
                    return NULL;
                }

                if (entry->m_type == ExprOperator::typeParenthesized)
                {
                    wchar_t* right = oper+entry->m_name.length();

                    wchar_t* pend = zl_stristr(right,
                                               NULL,
                                               entry->m_name_close,
                                               entry->m_alpha,
                                               entry->m_case_sense);

                    if (!pend)
                    {
                        // missing close operator
                        m_error_loc = right+wcslen(right);
                        m_error_code = errorMissingOperator;
                        m_error_text = entry->m_name_close;

                        delete inst;
                        return NULL;
                    }

                    tempc = *pend;
                    *pend = 0;
                    inst->m_params[1] = parseElement(penv, right, endloc);
                    *pend = tempc;
                    
                    if (endloc)
                        *endloc = (pend + entry->m_name_close.length());
                }
                 else
                {
                    wchar_t* right = oper+entry->m_name.length();
                    inst->m_params[1] = parseElement(penv, right, endloc);
                }

                if (!inst->m_params[1])
                {
                    delete inst;
                    return NULL;
                }

                inst->m_retval_type = getReturnValType(entry->m_formats.c_str(),
                                                       inst->m_params,
                                                       inst->m_param_count);
                return inst;
            }
        }
    }




    // give parse hook a chance
    if (m_parse_hook_mask & ExprParseHookInfo::typeIdentifier)
    {
        ExprParseHookInfo info;
        info.parser = this;
        info.penv = penv;
        info.hook_param = m_parse_hook_param;
        info.expr_text = ch;
        info.element_type = ExprParseHookInfo::typeIdentifier;
        info.res_element = NULL;
        
        bool accept;
        accept = m_parse_hook(info);

        if (accept)
        {
            return info.res_element;
        }
    }

    // give derived classes a chance
    {
        ExprElement* result = onParseElement(penv, expr, endloc);

        if (result || m_error_code != errorNone)
            return result;
    }


    Value* v;
     
    if (m_language == optionLanguageECMAScript)
    {
        if (isToken(ch, L"true"))
        {
            if (endloc) *endloc = ch+4;
            v = new Value;
            v->setBoolean(true);
            return static_cast<ExprElement*>(v);
        }
         else if (isToken(ch, L"false"))
        {
            if (endloc) *endloc = ch+5;
            v = new Value;
            v->setBoolean(false);
            return static_cast<ExprElement*>(v);
        }
         else if (isToken(ch, L"null"))
        {
            if (endloc) *endloc = ch+4;
            v = new Value;
            v->setNull();
            return static_cast<ExprElement*>(v);
        }
         else if (isToken(ch, L"this"))
        {
            if (endloc) *endloc = ch+4;
            ExprThis* t = new ExprThis;
            return static_cast<ExprElement*>(t);
        }
         else if (isToken(ch, L"super"))
        {
            if (endloc) *endloc = ch+5;
            
            ExprClassInfo* class_info = penv->getClassInfo();
            if (!class_info)
            {
                m_error_loc = ch;
                m_error_code = errorNotInClassScope;
                m_error_text = L"super";

                return NULL;
            }

            if (class_info->m_base_name.empty())
            {
                m_error_loc = ch;
                m_error_code = errorMissingBaseClass;
                m_error_text = L"super";

                return NULL;
            }



            ExprSuper* s = new ExprSuper;

            if (!class_info->m_base_name.empty())
            {
                s->m_base_class_info = penv->m_parser->getClassInfo(class_info->m_base_name);
            }

            return s;
        }
         else if (isToken(ch, L"arguments"))
        {
            if (endloc) *endloc = ch+9;
            ExprArguments* e = new ExprArguments;
            return e;
        }
    }
     else
    {
        // generic parser takes case insensitive true and false
        if (isTokenNoCase(ch, L"true"))
        {
            if (endloc) *endloc = ch+4;
            
            v = new Value;
            v->setBoolean(true);

            return static_cast<ExprElement*>(v);
        }
         else if (isTokenNoCase(ch, L"false"))
        {
            if (endloc) *endloc = ch+5;
            
            v = new Value;
            v->setBoolean(false);

            return static_cast<ExprElement*>(v);
        }
         else if (isTokenNoCase(ch, L"null"))
        {
            if (endloc) *endloc = ch+4;

            v = new Value;
            v->setNull();

            return static_cast<ExprElement*>(v);
        }
    }
    
    if (*ch == L'"' || *ch == L'\'')
    {
        wchar_t* string_end = NULL;
        v = (Value*)parseStringLiteral(penv, ch, &string_end, end-ch+1);
        if (!v)
            return NULL;
            
        // look for unexpected tokens after the string
        skipWhiteSpaceOrLS(string_end);
        if (endloc)
            *endloc = string_end;
        if (*string_end != 0)
        {
            delete v;
            m_error_loc = string_end;
            m_error_code = errorUnexpectedToken;
            m_error_text = L"";
            return NULL;
        }
        
        return v;
    }

    if (m_language == optionLanguageECMAScript && *ch == L'0' &&
        (towupper(*(ch+1)) == 'X' || iswdigit(*(ch+1))))
    {
        bool result;
        v = new Value;
        
        // parse hexidecimal or octal constant
        if (*(ch+1) == L'x' || *(ch+1) == L'X')
            result = parseHexOctConstant(ch+2, 16, v, (const wchar_t**)endloc);
             else
            result = parseHexOctConstant(ch+1, 8, v, (const wchar_t**)endloc);
            

        if (!result)
        {
            if (iswdigit(*(ch+1)))
            {
                // bad octal parse, try it as a decimal
                result = parseDecimalConstant(ch, v, (const wchar_t**)endloc);
                if (result)
                {
                    return static_cast<ExprElement*>(v);
                }
            }
        
            // bad hexidecimal or octal constant
            m_error_loc = ch;
            m_error_code = errorMalformedNumericConstant;
            m_error_text = L"";
            delete v;
            return NULL;
        }
        
        return static_cast<ExprElement*>(v);
    }
    
    if (iswdigit(*ch) || *ch == '.')
    {
        v = new Value;
        bool result = parseDecimalConstant(ch, v, (const wchar_t**)endloc);

        if (!result)
        {
            // malformed numeric constant
            m_error_loc = ch;
            m_error_code = errorMalformedNumericConstant;
            m_error_text = L"";
            delete v;
            return NULL;
        }
        
        return static_cast<ExprElement*>(v);
    }

    // try to look up variable

    ExprParserToken* var_token;
    std::wstring identifier = ch;
    
    wchar_t* p = ch;
    
    if (m_language == optionLanguageECMAScript)
    {
        // check if the first letter is a valid identifier character
        if (!isValidIdentifierFirstChar(*p))
        {
            if (endloc)
                *endloc = p;
            m_error_loc = p;
            m_error_code = errorUnexpectedToken;
            m_error_text = L"";
            return NULL;
        }
        
        while (*p)
        {
            if (p > ch && !isValidIdentifierChar(*p))
            {
                if (endloc)
                    *endloc = p;
                m_error_loc = p;
                m_error_code = errorUnexpectedToken;
                m_error_text = *p;
                return NULL;
            }
            ++p;
        }
    }
    
    var_token = penv->lookupToken(identifier);

    if (var_token)
    {
        ExprVarLookup* varlookup = new ExprVarLookup;
        varlookup->m_var_id = var_token->id;
        varlookup->m_symbol = identifier;
        if (var_token->binding)
        {
            varlookup->m_retval_type = var_token->binding->m_var_type;
        }

        return varlookup;
    }
     else
    {
        if (m_language == kscript::optionLanguageGeneric)
            return NULL;
    
        // try to resolve this after the whole file is parsed
        ExprVarLookup* varlookup = new ExprVarLookup;
        varlookup->m_parser = this;
        varlookup->m_location = ch;
        varlookup->m_symbol = ch;
        varlookup->m_scope = penv;
        
        addUnresolved(varlookup);

        return varlookup;
    }


    // symbol could not be found

    m_error_loc = ch;
    m_error_code = errorUndeclaredIdentifier;
    m_error_text = identifier;

    return NULL;
}






ExprElement* ExprParser::parseStatementElement(ExprParserEnv* penv,
                                               wchar_t* expr,
                                               wchar_t** endloc)
{
    wchar_t* end = NULL;
    wchar_t* end2 = NULL;
    
    while (iswspace(*expr))
        expr++;
    
    ExprElement* e = parseElement(penv, expr, &end);
    
    if (m_language != kscript::optionLanguageECMAScript ||
        (m_flags & optionNoAutoSemicolon))
    {
        // only ecma script has semicolon insertion
        if (e)
            end = expr+wcslen(expr);
        if (endloc)
            *endloc = end;
        return e;
    }

    
    if (e)
    {
        if (!end)
            end = expr+wcslen(expr);
    }
     else
    {
        if (!end || end == expr)
        {
            if (endloc)
                *endloc = end;
            return NULL;
        }
        
        
        // determine if we should insert a semicolon.  The rules for this
        // are described in ECMA-262 section 7.9.1
        bool insert_semicolon = false;
        
        if (isLineSeparator(*end) || *end == '}')
        {
            insert_semicolon = true;
        }
         else
        {
            wchar_t* p = end;
            while (iswspace(*p))
                ++p;
            while (p > expr)
            {
                p--;
                if (isLineSeparator(*p))
                {
                    insert_semicolon = true;
                    break;
                }
                if (!iswspace(*p))
                    break;
            }
        }
        
        // we need to insert a 'semicolon'.  This is actually
        // done by terminating the string before the offending
        // token and then reparsing.  This is done recursively
        // such that the actual statement is eventually reached.
        
        if (insert_semicolon)
        {
            m_error_loc = NULL;
            m_error_code = errorNone;
            m_error_text = L"";
            
            wchar_t tempc = *end;
            *end = 0;
            e = parseStatementElement(penv, expr, &end2);
            *end = tempc;
        }
    }
    
    if (endloc)
        *endloc = end;

    return e;
}
                     


inline wchar_t* lookForLabel(wchar_t* expr)
{
    if (!isValidIdentifierFirstChar(*expr))
        return NULL;
    ++expr;
    while (*expr)
    {
        if (*expr == ':')
            return expr;
        if (isWhiteSpace(*expr))
        {
            while (isWhiteSpace(*expr))
                ++expr;
            if (*expr == ':')
                return expr;
                 else
                return NULL;
        }
        if (!isValidIdentifierChar(*expr))
            return NULL;
        ++expr;
    }
    return NULL;
}


inline wchar_t* skipSemicolon(wchar_t* p)
{
    if (!p) return p;
    while (*p)
    {
        if (*p == ';') return p+1;
        if (!isWhiteSpaceOrLS(*p))
            break;
        ++p;            
    }
    return p;
}

ExprElement* ExprParser::parseStatement(ExprParserEnv* penv,
                                        wchar_t* expr,
                                        wchar_t** endloc)
{
    // strip off leading spaces
    while (iswspace(*expr))
        expr++;

    if (!*expr)
        return NULL;

    
    if (m_language == optionLanguageECMAScript)
    {
        // look for label
        int label_id = 0;
        
        wchar_t* colon = lookForLabel(expr);
        if (colon)
        {
            label_id = getSymbolId(xtrim(std::wstring(expr, colon-expr)));
            expr = colon+1;
            while (isWhiteSpaceOrLS(*expr))
                expr++;
        }
    
    
    
        if (*expr == L'{')
        {
            wchar_t* end = zl_findCloseBrace(expr);
            if (!end)
            {
                // missing close brace
                m_error_loc = NULL; // end
                m_error_code = errorMissingCloseBrace;
                m_error_text = L"";

                return NULL;
            }

            *end = 0;

            ExprSequence* e = parseSequence(penv, expr+1);
            if (e)
                e->m_label_id = label_id;
            *end = L'}';
            *endloc = end+1;
            return e;
        }


        // if conditional parser
        if (!wcsncmp(expr, L"if", 2) &&
            (iswspace(*(expr+2)) ||
            *(expr+2) == L'('))
        {
            expr = expr+2;
            while (iswspace(*expr))
                expr++;

            if (*expr != L'(')
            {
                // malformed if statement
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";

                return NULL;
            }


            wchar_t* end = zl_strchr(expr, L')');
            if (!end)
            {
                // malformed if statement
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";

                return NULL;
            }

            ExprIf* e_if = new ExprIf;
            wchar_t tempch = *(end+1);
            *(end+1) = 0;
            e_if->m_condition = parseElement(penv, expr);
            *(end+1) = tempch;

            if (!e_if->m_condition)
            {
                // bad condition
                delete e_if;
                return NULL;
            }

            // get the first (and perhaps only) statement
            e_if->m_iftrue = parseStatement(penv, end+1, &expr);

            if (!e_if->m_iftrue)
            {
                // malformed if statement
                delete e_if;
                return NULL;
            }

            while (iswspace(*expr))
                expr++;

            e_if->m_iffalse = NULL;

            if (!wcsncmp(expr, L"else", 4) && !isValidIdentifierChar(*(expr+4)))
            {
                // take care of the else statement
                expr += 4;
                e_if->m_iffalse = parseStatement(penv, expr, &expr);
                if (!e_if->m_iffalse)
                {
                    delete e_if;
                    return NULL;
                }
            }

            *endloc = expr;
            return e_if;
        }

        // 'switch' conditional parser
        if (!wcsncmp(expr, L"switch", 6) &&
            (iswspace(*(expr+6)) ||
            *(expr+6) == L'('))
        {
            expr = expr+6;
            while (*expr && iswspace(*expr))
                expr++;

            if (*expr != L'(')
            {
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }


            wchar_t* end = zl_strchr(expr, L')');
            if (!end)
            {
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }
            
            
            wchar_t* open_brace = end+1;
            while (*open_brace && iswspace(*open_brace))
                open_brace++;
            if (*open_brace != '{')
            {
                m_error_loc = open_brace; // end
                m_error_code = errorMissingOpenBrace;
                m_error_text = L"";
                return NULL;
            }

            wchar_t* close_brace = zl_findCloseBrace(open_brace);
            if (!close_brace)
            {
                m_error_loc = NULL; // end
                m_error_code = errorMissingCloseBrace;
                m_error_text = L"";
                return NULL;
            }

            wchar_t* first = open_brace+1;
            while (isWhiteSpaceOrLS(*first))
                ++first;
                
            if (!(nextTokenIs(first, L"case", 4) || nextTokenIs(first, L"default", 7) || !*first || *first == '}'))
            {
                // missing case or default clause
                m_error_loc = NULL; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }


            ExprSwitch* e_switch = new ExprSwitch;
            e_switch->m_label_id = label_id;
            wchar_t tempch = *(end+1);
            *(end+1) = 0;
            e_switch->m_expr = parseElement(penv, expr);
            *(end+1) = tempch;

            if (!e_switch->m_expr)
            {
                delete e_switch;
                return NULL;
            }
            


            *close_brace = 0;
            e_switch->m_sequence = parseSequence(penv, open_brace+1, e_switch);
            *close_brace = L'}';
            *endloc = close_brace+1;

            if (!e_switch->m_sequence)
            {
                delete e_switch;
                return NULL;
            }
            
            return e_switch;
        }



        // 'try' parser
        if (!wcsncmp(expr, L"try", 3) &&
            (iswspace(*(expr+3)) ||
            *(expr+3) == L'{'))
        {
            expr = expr+3;
            while (iswspace(*expr))
                expr++;

            ExprTryCatch* e_try = new ExprTryCatch;
            
            wchar_t *try_end, *catch_start = NULL, *catch_end,
                    *finally_start = NULL, *finally_end, *p;
            
            e_try->m_try_block = parseStatement(penv, expr, &try_end);
            if (!e_try->m_try_block)
            {
                delete e_try;
                return NULL;
            }
            
            
            
            p = try_end;
            while (iswspace(*p))
                p++;
            
            // find out if the next token is a 'catch' or a 'finally'
            if (nextTokenIs(p, L"catch", 5))
            {
                catch_start = p;
            }
             else if (nextTokenIs(p, L"finally", 7))
            {
                finally_start = p;
            }
             else
            {
                // missing catch statement
                m_error_loc = catch_start;
                m_error_code = errorMissingCatchFinally;
                m_error_text = L"";
                delete e_try;
                return NULL;
            }
            
            
            if (catch_start)
            {
                catch_start += 5;
                while (iswspace(*catch_start))
                    catch_start++;
                
                if (*catch_start != L'(')
                {
                    // malformed catch - missing open parenthesis
                    m_error_loc = catch_start;
                    m_error_code = errorMissingOpenParenthesis;
                    m_error_text = L"";
                    delete e_try;
                    return NULL;
                }
                
                wchar_t* close_paren = catch_start;
                while (*close_paren && *close_paren != ')')
                    close_paren++;
                
                if (*close_paren != L')')
                {
                    // malformed catch - missing close parenthesis
                    m_error_loc = catch_start;
                    m_error_code = errorMissingCloseParenthesis;
                    m_error_text = L"";
                    delete e_try;
                    return NULL;
                }
                
            
                // get the parameter
                wchar_t* param_start = catch_start+1;
                while (iswspace(*param_start))
                    param_start++;
                wchar_t* param_end = param_start;
                while (param_end < close_paren && !iswspace(*param_end))
                    param_end++;
                std::wstring param_text(param_start, param_end-param_start);
                
                if (param_text.length() == 0)
                {
                    // malformed catch - missing identifier
                    m_error_loc = catch_start+1;
                    m_error_code = errorUnexpectedToken;
                    m_error_text = L")";
                    delete e_try;
                    return NULL;
                }
                
                ExprParserEnv* catch_penv = new ExprParserEnv(penv);
                e_try->m_catch_param = catch_penv->getTokenId(param_text);
                
                
                catch_start = close_paren+1;
                e_try->m_catch_block = parseStatement(catch_penv,
                                                       catch_start,
                                                       &catch_end);
                if (!e_try->m_catch_block)
                {
                    delete e_try;
                    return NULL;
                }
                
                p = catch_end;
                while (iswspace(*p))
                    p++;
                if (nextTokenIs(p, L"finally", 7))
                    finally_start = p;
            }


            
            
            if (finally_start)
            {
                finally_start += 7;
                while (iswspace(*finally_start))
                    finally_start++;
                if (*finally_start != '{')
                {
                    // malformed finally - missing open brace
                    m_error_loc = finally_start;
                    m_error_code = errorMissingOpenBrace;
                    m_error_text = L"";
                    delete e_try;
                    return NULL;
                }
                
                e_try->m_finally_block = parseStatement(penv,
                                                        finally_start,
                                                        &finally_end);
                if (!e_try->m_finally_block)
                {
                    delete e_try;
                    return NULL;
                }

                *endloc = finally_end;
            }
             else
            {
                *endloc = catch_end;
            }
            
            return e_try;
        }

        // check for 'var'
        if (!wcsncmp(expr, L"var", 3) && isWhiteSpaceOrLS(*(expr+3)))
        {
            return parseVar(penv, expr, endloc);
        }
            
        // 'for' parser
        if (!wcsncmp(expr, L"for", 3) &&
            (iswspace(*(expr+3)) ||
            *(expr+3) == L'('))
        {
            expr = expr+3;
            while (isWhiteSpaceOrLS(*expr))
                expr++;

            if (*expr != L'(')
            {
                // malformed for statement
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";

                return NULL;
            }
            
            wchar_t* close_paren;
            close_paren = zl_strchr(expr, L')');
            if (!close_paren)
            {
                // malformed for statement
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";

                return NULL;
            }

            expr++;

            wchar_t* semicolon;
            wchar_t temps;
            
            // determine if semicolons were used in the "for" statement.
            // This will tell us whether it is a traditional for statement,
            // or a for..in statement
            temps = *(close_paren+1);
            *(close_paren+1) = 0;
            semicolon = zl_strchr(expr, L';');
            *(close_paren+1) = temps;
            
            
            if (!semicolon)
            {
                wchar_t* var_keyword;
                wchar_t* var_start;
                wchar_t* var_finish;
                wchar_t* in_start;
                wchar_t* in_finish;
                wchar_t* arr_start;
                wchar_t* arr_finish;

                wchar_t* pend;
                wchar_t close_temps;
                
                bool var_declaration = false;

                // look for a possible 'var' keyword
                var_keyword = zl_getNextToken(expr, &var_finish);
                if (!var_keyword)
                {
                    m_error_loc = expr; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";
                    return NULL;
                }
                if (var_finish - var_keyword == 3 &&
                    0 == wcsncmp(var_keyword, L"var", 3))
                {
                    var_declaration = true;
                }
                
                
                if (var_declaration)
                    var_start = var_finish;
                     else
                    var_start = expr;

                // find the variable name
                var_start = zl_getNextToken(var_start, &var_finish);
                if (!var_start)
                {
                    m_error_loc = expr; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";
                    return NULL;
                }
                


                in_start = zl_getNextToken(var_finish, &in_finish);
                if (!in_start ||
                    0 != wcsncmp(in_start, L"in", 2))
                {
                    // malformed for statement
                    if (in_start)
                        m_error_loc = in_start; // end
                         else
                        m_error_loc = expr; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";
                    return NULL;
                }

                close_temps = *close_paren;
                *close_paren = 0;
                arr_start = zl_getNextToken(in_finish, &arr_finish);
                *close_paren = close_temps;

                if (!arr_start)
                {
                    m_error_loc = expr; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";
                    return NULL;
                }


                expr = close_paren+1;


                ExprForIn* e_for = new ExprForIn;
                
                e_for->m_init = NULL;
                if (var_declaration)
                {
                    SetCharOnExitScope sc(var_finish, *var_finish, 0);
                    
                    e_for->m_init = parseStatement(penv, var_keyword, &pend);
                    if (!e_for->m_init)
                    {
                        m_error_loc = expr;
                        delete e_for;
                        return NULL;
                    }
                }
                

                {
                    SetCharOnExitScope sc(var_finish, *var_finish, 0);

                    e_for->m_element = parseElement(penv, var_start);
                    if (!e_for->m_element)
                    {
                        delete e_for;
                        return NULL;
                    }
                }
                
                {
                    SetCharOnExitScope sc(arr_finish, *arr_finish, 0);

                    e_for->m_arr = parseElement(penv, arr_start);
                    if (!e_for->m_arr)
                    {
                        delete e_for;
                        return NULL;
                    }
                }
                
                
                e_for->m_sequence = parseStatement(penv, expr, endloc);
                if (!e_for->m_sequence)
                {
                    delete e_for;
                    return NULL;
                }

                return e_for;
            }
             else
            {
                wchar_t* initend;
                
                while (iswspace(*expr))
                    expr++;
                
                ExprForWhile* e_for = new ExprForWhile;
                e_for->m_label_id = label_id;

                if (*expr != ';')
                {
                    e_for->m_init = parseStatement(penv, expr, &initend);
                    expr = initend;

                    if (!e_for->m_init)
                    {
                        delete e_for;
                        return NULL;
                    }
                }
                 else
                {
                    expr++;
                }
            
                semicolon = zl_strchr(expr, L';');
                if (!semicolon)
                {
                    // malformed for statement
                    m_error_loc = expr; // end
                    m_error_code = errorMalformedStatement;
                    m_error_text = L"";

                    delete e_for;
                    return NULL;
                }
                
                
                while (iswspace(*expr))
                    expr++;
                if (*expr != ';')
                {
                    *semicolon = 0;
                    e_for->m_condition = parseElement(penv, expr);
                    *semicolon = L';';
                    expr = semicolon+1;

                    if (!e_for->m_condition)
                    {
                        delete e_for;
                        return NULL;
                    }
                }
                 else
                {
                    expr++;
                }
                    
                while (iswspace(*expr))
                    expr++;
                if (*expr != L')')
                {
                    *close_paren = 0;
                    e_for->m_loop_oper = parseElement(penv, expr);
                    *close_paren = L')';

                    if (!e_for->m_loop_oper)
                    {
                        delete e_for;
                        return NULL;
                    }

                    expr = close_paren+1;
                }
                 else
                {
                    expr++;
                }
                
                e_for->m_sequence = parseStatement(penv, expr, endloc);

                if (!e_for->m_sequence)
                {
                    delete e_for;
                    return NULL;
                }

                return e_for;
            }
        }

        // 'while' parser
        if (!wcsncmp(expr, L"while", 5) &&
            (iswspace(*(expr+5)) ||
            *(expr+5) == L'('))
        {
            expr = expr+5;

            while (iswspace(*expr))
                expr++;

            if (*expr != L'(')
            {
                m_error_loc = expr; // end
                m_error_code = errorMissingOpenParenthesis;
                m_error_text = L"";

                return NULL;
            }
            
            wchar_t* close_paren;
            close_paren = zl_strchr(expr, ')');
            if (!close_paren)
            {
                // malformed while statement
                m_error_loc = expr; // end
                m_error_code = errorMissingCloseParenthesis;
                m_error_text = L"";

                return NULL;
            }

            *close_paren = 0;
            expr++;

            ExprForWhile* e_while = new ExprForWhile;
            e_while->m_init = NULL;
            e_while->m_condition = parseElement(penv, expr);
            e_while->m_loop_oper = NULL;
            e_while->m_label_id = label_id;
            *close_paren = L')';

            if (!e_while->m_condition)
            {
                delete e_while;
                return NULL;
            }
                    
            expr = close_paren+1;
            e_while->m_sequence = parseStatement(penv, expr, endloc);

            if (!e_while->m_sequence)
            {
                delete e_while;
                return NULL;
            }

            return e_while;
        }

        // 'do..while' parser --
        if (!wcsncmp(expr, L"do", 2) &&
            !isValidIdentifierChar(*(expr+2)))
        {
            expr = expr+2;

            while (iswspace(*expr))
                expr++;


            
            ExprForWhile* e_while = new ExprForWhile;
            e_while->m_init = NULL;
            e_while->m_condition_at_end = true;
            e_while->m_loop_oper = NULL;
            e_while->m_label_id = label_id;
            
            
            wchar_t* end_block;
            e_while->m_sequence = parseStatement(penv, expr, &end_block);
            
            if (!e_while->m_sequence)
            {
                delete e_while;
                return NULL;
            }
            
            // look for "while"
            wchar_t* p = end_block;
            
            while (iswspace(*p))
                ++p;
                
            if (0 != wcsncmp(p, L"while", 5))
            {
                m_error_loc = expr; // end
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                delete e_while;
                return NULL;
            }
            
            p += 5;
            while (iswspace(*p))
                p++;

            if (*p != L'(')
            {
                m_error_loc = p; // end
                m_error_code = errorMissingOpenParenthesis;
                m_error_text = L"";
                delete e_while;
                return NULL;
            }
            
            wchar_t* close_paren;
            close_paren = zl_strchr(p, ')');
            if (!close_paren)
            {
                m_error_loc = p; // end
                m_error_code = errorMissingCloseParenthesis;
                m_error_text = L"";
                delete e_while;
                return NULL;
            }
            
            *close_paren = 0;
            e_while->m_condition = parseElement(penv, p+1);
            *close_paren = L')';
            
            if (!e_while->m_condition)
            {
                delete e_while;
                return NULL;
            }

            
            p = close_paren+1;
            while (iswspace(*p))
                p++;
            
            if (*p == L';')
                p++;
            
            *endloc = p;
            
            /*
            // STRICT: require semicolon here
            if (*p != L';')
            {

                // malformed do/while statement
                m_error_loc = close_paren+1; // end
                m_error_code = errorMissingSemicolon;
                m_error_text = L"";
                delete e_while;
                return NULL;
            }

            *endloc = p+1;
            */
                       
            return e_while;
        }



        if (!wcsncmp(expr, L"with", 4) &&
            (isWhiteSpaceOrLS(*(expr+4)) ||
             *(expr+6) == L'('))
        {
            expr += 4;
            while (isWhiteSpaceOrLS(*expr))
                expr++;

            if (*expr != L'(')
            {
                m_error_loc = expr; // end
                m_error_code = errorMissingOpenParenthesis;
                m_error_text = L"";
                return NULL;
            }
            
            wchar_t* close_paren;
            close_paren = zl_strchr(expr, ')');
            if (!close_paren)
            {
                m_error_loc = expr; // end
                m_error_code = errorMissingCloseParenthesis;
                m_error_text = L"";
                return NULL;
            }

            *close_paren = 0;
            expr++;

            ExprWith* e_with = new ExprWith;
            e_with->m_with = parseElement(penv, expr, endloc);
            *close_paren = L')';
            
            if (!e_with->m_with)
            {
                delete e_with;
                return NULL;
            }
                   
            expr = close_paren+1;
            e_with->m_sequence = parseStatement(penv, expr, endloc);

            if (!e_with->m_sequence)
            {
                delete e_with;
                return NULL;
            }

            return e_with;
        }



        if (!wcsncmp(expr, L"return", 6) &&
            (!isValidIdentifierChar(*(expr+6)) ||
             *(expr+6) == 0))
        {
            expr += 6;
            while (iswspace(*expr))
                expr++;

            ExprElement* e = NULL;

            if (*expr == 0)
            {
                *endloc = expr;
            }
             else if (*expr != L';')
            {
                wchar_t* semicolon = zl_strchr(expr, L';', L"({[", L")}]");
                
                {
                    SetCharOnExitScope sc(semicolon, ';', 0);
                    e = parseStatementElement(penv, expr, endloc);
                }
                
                if (endloc && semicolon)
                {
                    // make sure the endloc is after the semicolon
                    wchar_t* ch = *endloc;
                    while (ch < semicolon && isWhiteSpaceOrLS(*ch))
                        ch++;
                    if (ch == semicolon)
                        ch++;
                    *endloc = ch;
                }
            }
             else
            {
                *endloc = expr+1;
            }

            
            ExprFunctionReturn* funcret = new ExprFunctionReturn;
            funcret->m_expr = e;
            
            return funcret;
        }

        if (!wcsncmp(expr, L"throw", 5) && iswspace(*(expr+5)))
        {
            expr += 5;
            while (iswspace(*expr))
                expr++;

            ExprElement* e;
            wchar_t* semicolon = zl_strchr(expr, L';', L"({[", L")}]");
            
            {
                SetCharOnExitScope sc(semicolon, ';', 0);
                e = parseStatementElement(penv, expr, endloc);
            }

            if (!e)
                return NULL;

            if (endloc && semicolon)
            {
                // make sure the endloc is after the semicolon
                wchar_t* ch = *endloc;
                while (ch < semicolon && isWhiteSpaceOrLS(*ch))
                    ch++;
                if (ch == semicolon)
                    ch++;
                *endloc = ch;
            }

            ExprThrow* ethrow = new ExprThrow;
            ethrow->m_expr = e;
            
            return ethrow;
        }

        if (!wcsncmp(expr, L"break", 5) &&
            (isWhiteSpaceOrLS(*(expr+5)) ||
            *(expr+5) == L';' ||
            *(expr+5) == 0))
        {
            expr += 5;
            while (isWhiteSpace(*expr))
                expr++;
            
            std::wstring label;
            if (!(*expr == ';' || isLineSeparator(*expr)))
            {
                // some kind of label
                while (*expr && *expr != ';' && !isLineSeparator(*expr))
                {
                    label += *expr;
                    expr++;
                }
                label = xtrim(label);
            }
            
            if (*expr)
                expr++;


/*
            // STRICT: if some kind of strict mode is to be implemented
            // regarding semicolons, it would go here
            
            if (*expr != L';')
            {
                m_error_loc = expr + wcslen(expr);
                m_error_code = errorMissingSemicolon;
                m_error_text = L"";

                return NULL;
            }
*/

            ExprBreak* funcbreak = new ExprBreak;
            if (label.length() > 0)
                funcbreak->m_label_id = getSymbolId(label);
                
            *endloc = expr;
            return funcbreak;
        }
        
        
        if (!wcsncmp(expr, L"continue", 8) &&
            (isWhiteSpaceOrLS(*(expr+8)) ||
            *(expr+8) == L';' ||
            *(expr+8) == 0))
        {
            expr += 8;
            while (isWhiteSpace(*expr))
                expr++;

            std::wstring label;
            if (!(*expr == ';' || isLineSeparator(*expr)))
            {
                // some kind of label
                while (*expr && *expr != ';' && !isLineSeparator(*expr))
                {
                    label += *expr;
                    expr++;
                }
                label = xtrim(label);
            }
            
            if (*expr)
                expr++;

/*
            // STRICT: if some kind of strict mode is to be implemented
            // regarding semicolons, it would go here
 
            if (*expr != L';')
            {
                m_error_loc = expr + wcslen(expr);
                m_error_code = errorMissingSemicolon;
                m_error_text = L"";

                return NULL;
            }
*/

            ExprContinue* funccont = new ExprContinue;
            if (label.length() > 0)
                funccont->m_label_id = getSymbolId(label);
            *endloc = expr;
            return funccont;
        }
        
        if (!wcsncmp(expr, L"use", 3) &&
            (isWhiteSpaceOrLS(*(expr+3))))
        {
            wchar_t* p = expr+3;
            while (isWhiteSpaceOrLS(*p))
                p++;
            if (!nextTokenIs(p, L"strict", 6))
            {
                m_error_loc = p;
                m_error_code = errorUnexpectedToken;
                m_error_text = L"";
                return NULL;
            }
            p += 6; // length of "strict";
            while (isWhiteSpaceOrLS(*p))
                p++;
            
            std::vector<std::wstring> options;
            wchar_t* start = p;
            while (*p && *p != ';' && !isLineSeparator(*p))
            {
                if (*p == ',')
                {
                    options.push_back(xtrim(std::wstring(start, p-start)));
                    start = p+1;
                }
                ++p;
            }
            
            if (p-start > 1)
            {
                options.push_back(xtrim(std::wstring(start, p-start)));
            }
            
            if (endloc)
            {
                if (*p)
                    *endloc = p+1;
                     else
                    *endloc = p;
            }
            
            std::vector<std::wstring>::iterator it;
            for (it = options.begin(); it != options.end(); ++it)
            {
                if (*it == L"semicolons")
                    setFlags(getFlags() | optionNoAutoSemicolon);
                else if (*it == L"variables")
                    setFlags(getFlags() | optionForceVarDeclare);
                else if (*it == L"classscoping")
                    setFlags(getFlags() | optionStrictClassScoping);
                else
                {
                    m_error_loc = p;
                    m_error_code = errorUnexpectedToken;
                    m_error_text = L"";
                    return NULL;
                }
            }
            
            return static_cast<ExprElement*>(new ExprEmptyStatement);
        }

        
        if (!wcsncmp(expr, L"include", 7) &&
            (iswspace(*(expr+7))))
        {
            wchar_t* fname = expr+8;
            while (iswspace(*fname))
                fname++;
                
            wchar_t quote_char = *fname;
            if (quote_char != L'\'' && quote_char != '"')
            {
                m_error_loc = fname;
                m_error_code = errorMissingQuote;
                m_error_text = L"";
                return NULL;
            }
            
            fname++;
            
            wchar_t* end_quote = fname;
            
            while (1)
            {
                if (*end_quote == quote_char)
                    break;
                    
                if (*end_quote == 0 || *end_quote == 0x0d || *end_quote == 0x0a)
                {
                    m_error_loc = end_quote;
                    m_error_code = errorMissingQuote;
                    m_error_text = L"";
                    return NULL;
                }
                
                end_quote++;
            }
            
            wchar_t* end_stmt = end_quote+1;
            while (iswspace(*end_stmt))
                end_stmt++;
            if (*end_stmt != L';')
            {
                m_error_loc = end_stmt;
                m_error_code = errorMissingSemicolon;
                m_error_text = L"";
                return NULL;
            }
            end_stmt++;

            
            ExprParseHookInfo info;
            info.parser = this;
            info.penv = penv;
            info.hook_param = m_parse_hook_param;
            info.expr_text = std::wstring(fname, end_quote-fname);
            info.element_type = ExprParseHookInfo::typeInclude;
            info.res_element = NULL;

            bool accept = false;
            
            if (m_parse_hook_mask & ExprParseHookInfo::typeInclude)
            {
                accept = m_parse_hook(info);
            }

            if (!accept)
            {
                // if the parse hook didn't already set the error code,
                // set a generic error code
                if (m_error_code == 0)
                {
                    m_error_loc = fname;
                    m_error_code = errorInvalidIncludeFile;
                    m_error_text = info.expr_text;
                }
                
                return NULL;
            }
            
            wchar_t* dup = wcsdup(info.res_string.c_str());
            if (!removeComments(dup))
            {
                free(dup);
                m_error_loc = NULL;
                m_error_code = errorUnterminatedComment;
                m_error_text = L"";
                return NULL;
            }
            
            ExprParserSource parse_src;
            parse_src.m_src = dup;
            parse_src.m_file = info.expr_text;
            m_sources.push_back(parse_src);
            
        
            ExprElement* res = parseSequence(penv, dup);
            if (res)
                *endloc = end_stmt;

            if (m_parse_hook_mask & ExprParseHookInfo::typeIncludePop)
            {
                ExprParseHookInfo info;
                info.parser = this;
                info.penv = penv;
                info.hook_param = m_parse_hook_param;
                info.expr_text = L"";
                info.element_type = ExprParseHookInfo::typeIncludePop;
                info.res_element = NULL;
                m_parse_hook(info);
            }

            
            return res;
        }


        if (wcschr(L")}]", *expr))
        {
            // unexpected character starting statement (usually too many
            // parenthesis following an if/while/for statement
            m_error_loc = expr;
            m_error_code = errorMalformedStatement;
            m_error_text = L"";
            m_error_text += *expr;
            return NULL;
        }
        
    } // if (m_language == optionLanguageECMAScript)




    if (endloc)
        *endloc = NULL;
        
    
    wchar_t* end;
    end = zl_strchr(expr, L';', L"({[", L")}]");
    if (end)
        *end = 0;
    
    if (m_language == optionLanguageECMAScript && end && expr == end)
    {
        // empty statement
        *end = L';';

        if (endloc)
            *endloc = end+1;
        
        return static_cast<ExprElement*>(new ExprEmptyStatement);
    }

    ExprElement* e = parseStatementElement(penv, expr, endloc);

    if (end)
        *end = L';';

    if (endloc)
    {
        *endloc = skipSemicolon(*endloc);
        
      //  if (end && *endloc == end)
      //      (*endloc)++;
    }

    return e;

    
    
}


ExprSequence* ExprParser::parseSequence(ExprParserEnv* parent_penv,
                                        wchar_t* expr,
                                        ExprSwitch* opt_switch,
                                        bool opt_eval)
{
    ExprSequence* sequence = new ExprSequence;

    ExprParserEnv* penv = new ExprParserEnv(parent_penv);
    penv->m_seq = sequence;
    penv->m_eval = opt_eval;  // true if this scope is an eval scope
    sequence->m_penv = penv;

    ExprElement* e;

    while (1)
    {
        while (iswspace(*expr))
            expr++;
            
        if (!*expr)
            break;

        if (nextTokenIs(expr, L"function", 8))
        {
            e = parseFunction(penv, expr, &expr, sequence);

            if (!e)
            {
                // parse failed; report failure to caller
                delete sequence;
                return NULL;
            }
             else
            {
                // function variable not used,
                // we can delete it now
                delete e;
                e = NULL;
            }
        }
         else if (nextTokenIs(expr, L"class", 5))
        {
            e = parseClass(penv, expr, &expr);

            if (!e)
            {
                // parse failed; report failure to caller
                delete sequence;
                return NULL;
            }

            // push class definitions onto the
            // beginning of the highest sequence
            penv->getHighestContext()->m_seq->prependElement(e);
        }
         else if (nextTokenIs(expr, L"case", 4))
        {
            e = parseCase(penv, expr, &expr, opt_switch, sequence->getElementCount());
            
            if (!e)
            {
                // parse failed; report failure to caller
                delete sequence;
                return NULL;
            }

            sequence->appendElement(e);
        }
         else if (nextTokenIs(expr, L"default", 7))
        {
            e = parseDefault(penv, expr, &expr, opt_switch, sequence->getElementCount());
            
            if (!e)
            {
                // parse failed; report failure to caller
                delete sequence;
                return NULL;
            }

            sequence->appendElement(e);
        }
         else
        {
            e = parseStatement(penv, expr, &expr);

            if (!e)
            {
                // parse failed; report failure to caller
                delete sequence;
                return NULL;
            }

            sequence->appendElement(e);
        }
    }
    
    return sequence;
}



ExprElement* ExprParser::parseObjectLiteral(ExprParserEnv* penv,
                                            wchar_t* expr,
                                            wchar_t** endloc)
{
    // strip off leading spaces
    while (iswspace(*expr))
        expr++;

    if (*expr != '{')
    {
        if (endloc)
            *endloc = expr;
        return NULL;
    }

    wchar_t* start = expr+1;
    wchar_t* end;
    wchar_t* p;
    wchar_t* close_brace;
    bool last = false;
    
    close_brace = zl_strchr(expr, '}', L"[({", L"])}");
    if (!close_brace)
    {
        if (endloc)
            *endloc = expr+wcslen(expr);
        m_error_loc = start;
        m_error_code = errorMissingCloseBrace;
        m_error_text = L"";
        return NULL;
    }
    
    end = close_brace;
    end++;
    if (endloc)
        *endloc = end;
        
        
    // look for garbage after terminator
    p = close_brace+1;
    while (iswspace(*p))
        p++;

    if (*p)
    {
        // garbage found after '}'
        if (endloc)
            *endloc = p;
        m_error_loc = end+1;
        m_error_code = errorMissingSemicolon;
        m_error_text = L"";
        return NULL;
    }


    SetCharOnExitScope sc(close_brace, '}');
    *close_brace = 0;
        
    
    ExprObjectLiteralInit* e = new ExprObjectLiteralInit;
    
    while (!last)
    {
        wchar_t* comma = zl_strchr(start, L',', L"([{", L")]}");
        if (!comma)
        {
            comma = end;
            last = true;
        }
        
        wchar_t tempch = *comma;
        *comma = 0;
            
        wchar_t* colon = zl_strchr(start, L':', L"([{", L")]}");
        
        if (last && !colon)
        {
            // empty json
            break;
        }
            
        if (!colon || colon == start)
        {
            *comma = tempch;
            
            m_error_loc = start;
            if (colon == start)
                m_error_code = errorMalformedStatement;
                 else
                m_error_code = errorMissingColon;
            m_error_text = L"";
            
            delete e;
            return NULL;
        }
        

        ExprElement* element = parseElement(penv, colon+1);
        *comma = tempch;
        if (!element)
        {
            // something went wrong in the child parse
            delete e;
            return NULL;
        }
        
        
        std::wstring identifier;
        
        // parse the identifier before the colon
        {
            SetCharOnExitScope sc(colon, ':', 0);
            
            wchar_t* ch = start;
            while (isWhiteSpaceOrLS(*ch))
                ++ch;
            if (*ch == '"' || *ch == '\'')
            {
                wchar_t* endloc = NULL;
                Value* v = (Value*)parseStringLiteral(penv, ch, &endloc);
                if (!v)
                    return NULL;
                identifier = v->getString();
                delete v;
            }
             else
            {
                identifier = std::wstring(start, colon-start);
            }                            
        }
        
        
        ExprObjectLiteralItem item;
        item.m_name = xtrim(identifier);
        item.m_varinit = element;
        
        e->m_elements.push_back(item);
        
        start = comma+1;
    }

    return e;
}

ExprElement* ExprParser::parseArrayLiteral(ExprParserEnv* penv,
                                           wchar_t* expr,
                                           wchar_t** endloc)
{
    // strip off leading spaces
    while (iswspace(*expr))
        expr++;

    if (*expr != '[')
    {
        if (endloc)
            *endloc = expr;
        return NULL;
    }

    wchar_t* start = expr+1;
    wchar_t* end;
    wchar_t* p;
    wchar_t* close_bracket;
    bool last = false;
    
    // find end
    close_bracket = zl_strchr(expr, ']', L"[({", L"])}");
    if (!close_bracket)
    {
        m_error_loc = start;
        m_error_code = errorMissingCloseBracket;
        m_error_text = L"";
        if (endloc)
            *endloc = start;
        return NULL;
    }
    
    end = close_bracket;
    end++;
    if (endloc)
        *endloc = end;
    
    
    // look for garbage after terminator   
    p = close_bracket+1;
    while (isWhiteSpace(*p))
        p++;

    if (*p)
    {
        // garbage found after ']'
        if (endloc)
            *endloc = p;
        m_error_loc = end+1;
        m_error_code = errorMissingSemicolon;
        m_error_text = L"";
        return NULL;
    }


    SetCharOnExitScope sc(close_bracket, ']', 0);
        
    
    ExprArrayInit* e = new ExprArrayInit;

    // check for empty array initializer
    p = start;
    while (isWhiteSpace(*p))
        p++;
    if (!*p)
    {
        // empty initializer
        return e;
    }

    while (!last)
    {
        wchar_t* comma = zl_strchr(start, L',', L"([{", L")]}");
        if (!comma)
        {
            comma = end;
            last = true;
        }

        wchar_t tempch = *comma;
        *comma = 0;
        
        while (*start && isWhiteSpaceOrLS(*start))
            start++;
        
        if (!*start)
        {
            // empty element;
            Value* v = new Value;
            v->setUndefined();
            e->m_elements.push_back(v);
            start++;
            *comma = tempch;
            continue;
        }
            
        
        ExprElement* element = parseElement(penv, start);
        *comma = tempch;
        
        if (!element)
        {
            // something went wrong in the child parse
            delete e;
            return NULL;
        }
        
        e->m_elements.push_back(element);
        
        start = comma+1;
    }

    return e;
}


ExprElement* ExprParser::parseCase(ExprParserEnv* penv,
                                   wchar_t* expr,
                                   wchar_t** endloc,
                                   ExprSwitch* switch_element,
                                   size_t sequence_offset)
{    
    if (!switch_element || !nextTokenIs(expr, L"case", 4))
    {
        m_error_loc = expr;
        m_error_code = errorUnexpectedToken;
        m_error_text = L"";
        return NULL;
    }
    
    expr += 4;
    while (*expr && iswspace(*expr) && *expr != L':')
        expr++;
        
    wchar_t* colon = zl_strchr(expr, L':', L"({[", L")}]");
    if (!colon)
    {
        m_error_loc = expr;
        m_error_code = errorMissingColon;
        m_error_text = L"";
        return NULL;
    }
    

    *colon = 0;
    ExprElement* e = parseElement(penv, expr);
    *colon = L':';
    
    if (!e)
        return NULL;
        
    ExprSwitchCase c;
    c.case_element = e;
    c.sequence_offset = sequence_offset;
    switch_element->m_cases.push_back(c);



    expr = colon+1;
    while (*expr && iswspace(*expr))
        expr++;
    if (nextTokenIs(expr, L"case", 4) || nextTokenIs(expr, L"default", 7) || !*expr)
    {
        *endloc = expr;
        ExprEmptyStatement* e = new ExprEmptyStatement;
        return e;
    }
    
    
    return parseStatement(penv, expr, endloc);
}

ExprElement* ExprParser::parseDefault(ExprParserEnv* penv,
                                      wchar_t* expr,
                                      wchar_t** endloc,
                                      ExprSwitch* switch_element,
                                      size_t sequence_offset)
{    
    if (!switch_element || !nextTokenIs(expr, L"default", 7) || ++switch_element->m_default_count > 1)
    {
        m_error_loc = expr;
        m_error_code = errorUnexpectedToken;
        m_error_text = L"";
        return NULL;
    }
    
    expr += 7;
    while (*expr && iswspace(*expr) && *expr != L':')
        expr++;
        
    wchar_t* colon = zl_strchr(expr, L':', L"({[", L")}]");
    if (!colon)
    {
        m_error_loc = expr;
        m_error_code = errorMissingColon;
        m_error_text = L"";
        return NULL;
    }
    

    ExprSwitchCase c;
    c.case_element = NULL;
    c.sequence_offset = sequence_offset;
    switch_element->m_cases.push_back(c);


    expr = colon+1;
    while (*expr && iswspace(*expr))
        expr++;
    
    if (nextTokenIs(expr, L"case", 4) || !*expr)
    {
        *endloc = expr;
        ExprEmptyStatement* e = new ExprEmptyStatement;
        return e;
    }
    
    return parseStatement(penv, colon+1, endloc);
}
                  

ExprElement* ExprParser::parseFunction(ExprParserEnv* penv,
                                        wchar_t* expr,
                                        wchar_t** endloc,
                                        ExprSequence* seq,
                                        bool function_expr)
{
    if (0 != wcsncmp(expr, L"function", 8) || iswalpha(*(expr+8)))
        return NULL;
        

    // get the class info; if the function is not being defined
    // inside of a class, class_info will be null
    ExprClassInfo* class_info = penv->getClassInfo();
    
    wchar_t* func_begin = expr;
    
    expr += 8;
    while (iswspace(*expr))
        expr++;

    wchar_t* paren = wcschr(expr, L'(');
    if (paren == NULL)
    {
        m_error_loc = expr; // end
        m_error_code = errorMalformedStatement;
        m_error_text = L"";

        // malformed function declaration
        return NULL;
    }

    *paren = 0;
    std::wstring func_name = xtrim(expr);
    std::wstring identifier = func_name;
    *paren = L'(';

    
    if (identifier.length() == 0)
    {
        // anonymous function; this doesn't belong in a class definition
        class_info = NULL;
    }
     else
    {
        // make sure the function name is a valid identifier
        if (!isValidIdentifier(identifier))
        {
            m_error_loc = expr;
            m_error_code = errorInvalidIdentifier;
            m_error_text = L"";
            return NULL;
        }
    }



    if (class_info)
    {
        identifier = class_info->m_name + L"::" + identifier;
    }

    // check to see if the function has already been declared
    ExprParserEnv* var_penv = penv->getVarContext();
    ExprParserEnv* eval_penv = penv->getEvalContext();


    if ((m_flags & optionForceVarDeclare) && identifier.length() > 0)
    {        
        if (var_penv->getTokenExistInScope(identifier))
        {
            // variable/function by this name
            // has already been declared
            m_error_loc = expr;
            m_error_code = errorVarRedefinition;
            m_error_text = identifier;
      
            return NULL;
        }
    }



    // parse the parameters, if any
    wchar_t* next_param = paren+1;
    wchar_t* param_text;
    bool res;

    ExprParserEnv* new_penv = new ExprParserEnv(penv);
    Function* funcdef = createFunction();
    new_penv->m_func = funcdef;
    

    while (1)
    {
        wchar_t* param_end;
        wchar_t temp_paramc;
        res = zl_getParam(&next_param,
                          &param_text,
                          &param_end,
                          L"(",
                          L")");

        if (!param_text)
            break;

        temp_paramc = *param_end;
        *param_end = 0;
        

        if ((m_flags & optionForceVarDeclare) && new_penv->getTokenExistInScope(param_text))
        {
            // parameter by this name has already been declared
            m_error_loc = param_text;
            m_error_code = errorVarRedefinition;
            m_error_text = param_text;

            *param_end = temp_paramc;

            delete funcdef;
            return NULL;
        }

        
        funcdef->m_param_ids.push_back(new_penv->getTokenId(param_text));
        *param_end = temp_paramc;

        if (!res)
            break;
    }

    paren = zl_strchr(paren, L')');
    if (paren == NULL)
    {
        // malformed function declaration
        m_error_loc = expr;
        m_error_code = errorMissingCloseParenthesis;
        m_error_text = L"";
        delete funcdef;
        return NULL;
    }
    
    
    
    // parse the function body
    wchar_t* body = paren+1;
    while (iswspace(*body))
        body++;
    if (*body != '{')
    {
        m_error_loc = paren+1; // end
        m_error_code = errorMissingOpenBrace;
        m_error_text = L"";
        delete funcdef;
        return NULL;
    }
    
    wchar_t* end = zl_findCloseBrace(expr);
    if (!end)
    {
        m_error_loc = NULL; // end
        m_error_code = errorMissingCloseBrace;
        m_error_text = L"";
        delete funcdef;
        return NULL;
    }
    
    
    // functions are parsed either as function declarations or
    // function expressions.  For automatic semicolon insertion
    // to work properly, we should return an error if there are
    // unwanted tokens after a function expression;  for function
    // declarations, this check is unnecessary
    
    if (function_expr)
    {
        // are there unwanted tokens after the function body?
        
        wchar_t* p = end+1;
        while (iswspace(*p))
            p++;

        if (*p)
        {
            if (endloc)
                *endloc = p;

            m_error_loc = end+1;
            m_error_code = errorMissingSemicolon;
            m_error_text = L"";
            delete funcdef;
            return NULL;
        }
    }
    

    *end = 0;
    funcdef->m_sequence = parseSequence(new_penv, body+1);
    *end = L'}';
    
    if (funcdef->m_sequence == NULL)
    {
        // something bad happened while parsing the function
        delete funcdef;
        return NULL;
    }
    
    funcdef->m_sequence->m_create_env = false;

    funcdef->m_text.assign(func_begin, end-func_begin+1);
    
    if (endloc)
        *endloc = end+1;
    
    new_penv->m_seq = (ExprSequence*)funcdef->m_sequence;



    // set the length parameter of the function
    Value* len = funcdef->getMember(L"length");
    len->setInteger((int)funcdef->m_param_ids.size());
    len->setAttributes(Value::attrReadOnly);



    if (identifier.length() > 0)
    {
        if (class_info)
        {
            // add class method to the constructor
            //Value* val = new Value;
            //val->setFunction(funcdef);
            
            //class_info->addMember(func_name, val);
            class_info->m_prototype.createMember(func_name)->setFunction(funcdef);
            

            // if this function happens to be named the same thing as the
            // class, it is a constructor method.  Check to see if it
            // called 'super'. If it didn't, and the class is derived,
            // prepend an implicit constructor call

            if (func_name == class_info->m_name || func_name == L"constructor")
            {
                class_info->m_cstdef = funcdef;
                
                if (class_info->m_base_name.length() > 0 &&
                    !new_penv->getSuperCalled())
                {
                    ExprSequence* func_seq = (ExprSequence*)funcdef->m_sequence;

                    ExprSuper* s = new ExprSuper;
                    s->m_base_class_info = penv->m_parser->getClassInfo(class_info->m_base_name);
                
                    ExprFunctionCall* call = new ExprFunctionCall;
                    call->m_func_ptr = s;
                    call->m_retval_type = Value::typeUndefined;
                    call->m_param_count = 0;

                    func_seq->prependElement(call);
                }
            }
        }
         else
        {
            ExprVarDefine* def = new ExprVarDefine;
            def->m_init = new ExprFunctionInit(funcdef);
            def->m_var_id = var_penv->getTokenId(identifier);

            var_penv->getSequence()->addFunctionElement(def);
            
            
            if (eval_penv && eval_penv != var_penv)
            {
                ExprVarDefine* def = new ExprVarDefine;
                def->m_init = new ExprFunctionInit(funcdef);
                def->m_var_id = var_penv->getTokenId(identifier);
                def->m_scope = var_penv;
                
                eval_penv->getSequence()->addFunctionElement(def);
            }
        }
    }

    return (new ExprFunctionInit(funcdef));
}




ExprElement* ExprParser::parseRegexLiteral(ExprParserEnv* penv,
                                        wchar_t* expr,
                                        wchar_t** endloc)
{
    if (*expr != '/')
        return NULL;
    
    std::wstring flags;
    
    wchar_t* regex_end = NULL;
    
    wchar_t* p = expr+1;
    while (*p)
    {
        if (*p == '\\')
        {
            p += 2;
            continue;
        }
        
        if (*p == '/')
        {
            regex_end = p;
            break;
        }
        ++p;
    }
    
    if (!regex_end)
    {
        if (endloc)
            *endloc = p;
        m_error_loc = p;
        m_error_code = errorSyntaxError;
        m_error_text = L"";
        return NULL;
    }
        
    wchar_t* f = regex_end+1;
    while (*f && isValidIdentifierChar(*f))
    {
        if (*f == L'g' || *f == L'i' || *f == L'm')
            flags += *f;
        ++f;
    }
    
    if (endloc)
        *endloc = f;
    
    RegExp* r = RegExp::createObject(this);
    if (!r->assign(std::wstring(expr+1, regex_end), flags))
    {
        if (endloc)
            *endloc = expr+1;
        m_error_loc = expr+1;
        m_error_code = errorSyntaxError;
        m_error_text = L"";
        delete r;
        return NULL;
    }
    
    Value* v = new Value;
    v->setObject(r);
    
    return v;
}


ExprElement* ExprParser::parseVar(ExprParserEnv* penv,
                                   wchar_t* expr,
                                   wchar_t** endloc,
                                   ExprSequence* seq)
{
    // if we aren't dealing with a var, get out
    if (!nextTokenIs(expr, L"var", 3))
        return NULL;
    
    // find out the parser environment (scope) where the variable
    // declaration belongs
    ExprParserEnv* var_penv = penv->getVarContext();
    ExprParserEnv* eval_penv = penv->getEvalContext();

    if (!var_penv)
        return NULL;
    
    // skip forward to the first definition
    expr += 3;
    
    
    // look for the ending semicolon
    wchar_t* semicolon = zl_strchr(expr, L';', L"([{", L")]}");
    SetCharOnExitScope sc_semicolon(semicolon, ';', 0);


    // create a sequence for the '=' portions
    ExprSequence* sequence = new ExprSequence;

    wchar_t* end = NULL;

    bool first = true;
    while (1)
    {
        while (isWhiteSpaceOrLS(*expr))
            expr++;

        if (!first)
        {
            if (*expr != ',')
                break;
            expr++;
            while (isWhiteSpaceOrLS(*expr))
                expr++;
        }
        

        // block off next zero-level comma, if found
        wchar_t* comma = zl_strchr(expr, L',', L"([{", L")]}");
        SetCharOnExitScope sc_comma(comma, ',', 0);

        // check if first character is a valid one
        if (!isValidIdentifierFirstChar(*expr))
        {
            if (endloc)
                *endloc = expr;
            m_error_loc = expr;
            m_error_code = errorInvalidIdentifier;
            m_error_text = L"";
            delete sequence;
            return NULL;
        }

        // find the end of the identifier
        wchar_t* ident_begin = expr;
        wchar_t* ident_end = expr+1;
        
        while (*ident_end)
        {
            if (!isValidIdentifierChar(*ident_end))
                break;
            ident_end++;
        }
        
        // look for an equals sign; we must encounter
        // a string terminator, an '=', or a ',';
        // comma was turned temporarily into a null above
        wchar_t* eq = ident_end;
        wchar_t* line_separator = NULL;
        while (iswspace(*eq))
        {
            if (isLineSeparator(*eq))
                line_separator = eq;
            eq++;
        }
        
        if (*eq != 0 && *eq != '=' && !line_separator)
        {
            if (endloc)
                *endloc = eq;
            m_error_loc = eq;
            m_error_code = errorUnexpectedToken;
            m_error_text = *eq;
            delete sequence;
            return NULL;
        }
        
        // create a string of the identifier
        std::wstring identifier(ident_begin, ident_end - ident_begin);
        
        // check if it's a valid identifier (this will also
        // check against reserved keywords)
        
        if (!isValidIdentifier(identifier))
        {
            if (endloc)
                *endloc = ident_begin;
            m_error_loc = ident_begin;
            m_error_code = errorInvalidIdentifier;
            m_error_text = L"";
            delete sequence;
            return NULL;
        }
        
        
        
        // parse the variable declaration
        ExprVarDefine* vardef = new ExprVarDefine;
        vardef->m_var_id = var_penv->getTokenId(identifier);
        var_penv->m_seq->addVarElement(vardef);

        if (eval_penv && eval_penv != var_penv)
        {
            ExprVarDefine* vardef = new ExprVarDefine;
            vardef->m_var_id = var_penv->getTokenId(identifier);
            vardef->m_scope = var_penv;
            eval_penv->getSequence()->addVarElement(vardef);
        }
        
        
        if (*eq == '=')
        {
            ExprVarAssign* e = new ExprVarAssign;
            e->m_var_id = penv->getTokenId(identifier);
            e->m_init = parseStatementElement(penv, eq+1, &end);
            if (!e->m_init)
            {
                delete e;
                delete sequence;
                return NULL;
            }
            
            sequence->appendElement(e);
            expr = end;
        }
         else
        {
            if (comma)
            {
                end = comma+1;
                expr = comma;
            }
             else
            {
                end = ident_end+1;
                expr = end;
            }
            
        }

        first = false;
    }
    

    if (endloc)
    {
        if (end)
        {

            if (semicolon)
            {
                // get rid of whitespace between end of statement and semicolon
                while (*end && iswspace(*end) && end < semicolon)
                    end++;
                if (end == semicolon)
                    end++;
            }
            *endloc = end;
        }
         else
        {
            *endloc = semicolon+1;
        }
    }
    
    return sequence;
}
       
ExprElement* ExprParser::parseClass(ExprParserEnv* penv,
                                    wchar_t* expr,
                                    wchar_t** endloc)
{
    if (!nextTokenIs(expr, L"class", 5))
        return NULL;
        
    expr += 5;
    while (iswspace(*expr))
        expr++;
    
    wchar_t* brace = wcschr(expr, L'{');
    if (!brace)
    {
        // malformed class declaration
        m_error_loc = expr; // end
        m_error_code = errorMalformedStatement;
        m_error_text = L"Missing brace";
        return NULL;
    }

    *brace = 0;

    std::wstring dec = xtrim(expr);
    std::wstring class_name;
    std::wstring baseclass_name;

    size_t pos = dec.find(L"extends");
    if (pos == dec.npos)
    {
        class_name = xtrim(expr);
    }
     else
    {
        class_name = xtrim(dec.substr(0, pos));
        baseclass_name = xtrim(dec.substr(pos+7));
    }


    *brace = L'{';


    wchar_t* end = zl_findCloseBrace(expr);
    if (!end)
    {
        m_error_loc = NULL;
        m_error_code = errorMissingCloseBrace;
        m_error_text = L"";

        return NULL;
    }
    
    
    // determine if the class already exists
    if (penv->m_parser->getClassExist(class_name))
    {
        m_error_loc = expr;
        m_error_code = errorVarRedefinition;
        m_error_text = class_name;

        return NULL;
    }


    // determine if the base class exists
    if (baseclass_name.length() > 0 && !penv->m_parser->getClassExist(baseclass_name))
    {
        m_error_loc = expr+pos+8;
        m_error_code = errorUndeclaredIdentifier;
        m_error_text = baseclass_name;

        return NULL;
    }
    
    
    // initialize class constructor
    ExprClassInfo* class_info = penv->m_parser->getClassInfo(class_name);
    ExprClassInfo* base_info = penv->m_parser->getClassInfo(baseclass_name);
    //ExprClassInfo* class_info = penv->m_parser->addClass(class_name, baseclass_name);
    class_info->m_base_name = baseclass_name;

    
    if (base_info)
    {
        // make sure to chain prototypes together for derived classes
        class_info->m_prototype.setPrototype(&base_info->m_prototype);
    }
    


    ExprParserEnv* new_penv;
    new_penv = new ExprParserEnv(penv);
    new_penv->setClass(class_name);
    penv = new_penv;


    // parse the class sequence
    *end = 0;

    {
        expr = brace+1;
        while (1)
        {
            while (iswspace(*expr))
                expr++;
            
            if (!*expr)
                break;

            if (nextTokenIs(expr, L"function", 8))
            {
                ExprElement* e;
                e = parseFunction(penv, expr, &expr);

                if (e)
                    delete e;
                     else
                    return NULL;
            }
             else if (nextTokenIs(expr, L"var", 3))
            {
                expr += 3;
                while (iswspace(*expr))
                    expr++;

                wchar_t tempc;
                wchar_t* semicolon;
                wchar_t* endc;
                wchar_t* eq;

                semicolon = zl_strchr(expr, L';', L"([", L")]");
                if (!semicolon)
                {
                    // malformed var statement, missing semicolon
                    m_error_loc = expr; // end
                    m_error_code = errorMissingSemicolon;
                    m_error_text = L"";

                    return NULL;
                }

                *semicolon = 0;

                bool last = false;
                while (1)
                {
                    endc = zl_strchr(expr, L',', L"([", L")]");
                    if (!endc)
                    {
                        endc = expr + wcslen(expr);
                        last = true;
                    }

                    tempc = *endc;
                    *endc = 0;

                    while (iswspace(*expr))
                        expr++;

                    eq = wcschr(expr, L'=');
                    if (eq)
                        *eq = 0;

                    std::wstring identifier = xtrim(expr);

                    if (eq)
                        *eq = L'=';

                    if (class_info->getMemberExists(identifier))
                    {
                        // variable already defined
                        m_error_loc = expr;
                        m_error_code = errorVarRedefinition;
                        m_error_text = identifier;

                        *endc = tempc;
                        *semicolon = L';';

                        return NULL;
                    }

                    ExprElement* varinit = NULL;
                                            
                    if (eq)
                    {
                        varinit = parseElement(penv, eq+1);
                        if (!varinit)
                        {
                            *endc = tempc;
                            *semicolon = L';';
                            return NULL;
                        }
                    }

                    class_info->addMember(identifier, varinit);

                    *endc = tempc;

                    if (last)
                        break;

                    expr = endc+1;
                }

                *semicolon = L';';
                expr = semicolon+1;
            }
             else
            {
                // variable already defined
                m_error_loc = expr;
                m_error_code = errorMalformedStatement;
                m_error_text = L"";
                return NULL;
            }
        }
    }

    *end = L'}';
    if (endloc)
        *endloc = end+1;


    // determine whether or not the class defined a
    // constructor method
    
    ExprParserEnv* highest_penv = penv->getHighestContext();

    Function* classdef = createFunction();
    classdef->m_func_ptr = (ExprBindFunc)kscript_object_constructor;
    classdef->m_binding_param = (void*)class_info;
    classdef->m_defer_calc = false;

    if (class_info->getMemberExists(class_name))
    {
        // yes, we have a constructor method
        Value v;
        class_info->m_members[class_name]->eval(NULL, &v);
        class_info->m_cstdef = v.m_funcptrval;
    }

    Value* val = new Value;
    val->setFunction(classdef);


    class_info->m_class_obj = val;

    ExprVarDefine* vardef = new ExprVarDefine;
    vardef->m_var_id = highest_penv->getTokenId(class_name);
    vardef->m_init = val;

    return vardef;
}



class GlobalObject : public kscript::Object
{
public:

    GlobalObject()
    {
        m_parser = NULL;
        m_global_scope = NULL;
    }
    
    void initGlobalObject(ExprParser* parser, ExprEnv* scope)
    {
        m_parser = parser;
        m_global_scope = scope;
    }
    
    void toString(Value* retval)
    {
        retval->setString(L"[object global]");
    }

    Value* lookupMember(const std::wstring& name, bool follow_prototype)
    {
        return m_global_scope->getValue(m_parser->getSymbolId(name));
    }

    Value* createMember(const std::wstring& name, unsigned int props)
    {
        return m_global_scope->declareVar(m_parser->getSymbolId(name));
    }
    
    std::wstring lookupMember(Value* value, bool follow_prototype)
    {
        if (m_global_scope->m_min_var != -1)
        {
            for (int i = m_global_scope->m_min_var; i <= m_global_scope->m_max_var; ++i)
            {
                if (m_global_scope->getValue(i) == value)
                    return m_parser->getSymbolName(i);
            }
        }
        
        return L"";
    }

    bool deleteMember(const std::wstring& name)
    {
        Value* v = m_global_scope->getValue(m_parser->getSymbolId(name));
        if (!v)
            return true;
        
        if (v->getDontDelete())
            return false;
        
        v->reset();
        return true;
    }

private:

    ExprParser* m_parser;
    ExprEnv* m_global_scope;
};



ExprParser::ExprParser(unsigned int flags)
{
    // initialize some variables
    m_entrypt = NULL;
    m_global_scope = NULL;
    m_parse_hook = NULL;
    m_parse_hook_param = NULL;
    m_parse_hook_mask = 0;
    m_retval_type = Value::typeNull;
    m_bindings_defined = false;
    m_cancelled = false;
    m_symbol_counter = 1;
    m_extra_long = 0;
    m_root_penv = NULL;
    m_flags = flags;
    m_label_id = 0;
    m_global_scope = NULL;
#ifdef _DEBUG
    m_max_depth = 500;
#else
    m_max_depth = 1500;
#endif
    m_current_depth = 0;

    m_error_code = errorNone;
    m_error_line = 0;
    m_error_loc = NULL;
    m_error_text = L"";
    
    m_language = 0;
    
    if (flags & optionLanguageCustom)
        m_language = optionLanguageCustom;
    else if (flags & optionLanguageGeneric)
        m_language = optionLanguageGeneric;
    else
        m_language = optionLanguageECMAScript;  // default
        
    if (flags & optionEpsilonNumericCompare)
        dblcompare = kscript::dblcompare;
         else
        dblcompare = kscript::dblcompare_strict; // default
        
    
    // manually add Object and Function classes;  this needs
    // to be done manually because any other bindings depend
    // on these having already been initialized   
    m_object_prototype.setObject(new ValueObject);
    
    ExprClassInfo* obj_class = getClassInfo(L"Object");
    obj_class->m_prototype.setValue(m_object_prototype);
    
    Function* function_prototype = new Function;
    m_function_prototype.setObject(function_prototype);
    function_prototype->setPrototype(&m_object_prototype);
    
    ExprClassInfo* function_class = getClassInfo(L"Function");
    function_class->m_base_name = L"Object";
    function_class->m_prototype.setValue(m_function_prototype);

    
    if (m_language == optionLanguageECMAScript)
    {
        
        if (!(flags & optionNoAutoBindings))
        {
            m_opers.reserve(50);
            addOperator(L"[", L"]", true, oper_array,           130, ExprOperator::typeParenthesized, ExprOperator::directionRightToLeft, true, L"*(*i);*(*s)");
            addOperator(L"(", L")", true, NULL,                 130, ExprOperator::typeParenthesized, ExprOperator::directionRightToLeft, true, L"*(*i);*(*s)");
            addOperator(L".",       true, oper_array,           130, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true, L"*(*i);*(*s)");
            addOperator(L"~",       true, oper_bitwise_not,     120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, false, L"i(i);f(f)");
            addOperator(L"!",       true, oper_not,             120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, false, L"b(b)");
            addOperator(L"++",      true, oper_increment,       120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, true, L"n(n)");
            addOperator(L"--",      true, oper_decrement,       120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, true, L"n(n)");
            addOperator(L"delete",  true, oper_delete,          120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, true, L"*(*)");
            addOperator(L"void",    true, oper_void,            120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, true, L"*(*)");
            addOperator(L"typeof",  true, oper_typeof,          120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, true, L"s(*)");
            addOperator(L"-",       true, oper_negate,          120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, false, L"f(n);f(s);f(b)");
            addOperator(L"+",       true, oper_unary_plus,      120, ExprOperator::typeUnary,  ExprOperator::directionLeftToRight, false, L"f(n);f(s);f(b)");
            addOperator(L"%",       true, oper_mod,             100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"/",       true, oper_divide,          100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"*",       true, oper_multiply,        100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"+",       true, oper_add,              90, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn);s(ss);s(s*);s(*s)");
            addOperator(L"-",       true, oper_subtract,         90, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"<<",      true, oper_shift_left,       85, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"n(nn)");
            addOperator(L">>",      true, oper_shift_right,      85, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"n(nn)");
            addOperator(L">>>",     true, oper_shift_right_zf,   85, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"n(nn)");
            addOperator(L"<",       true, oper_less_than,        80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss)");
            addOperator(L"<=",      true, oper_less_than_eq,     80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss)");
            addOperator(L">",       true, oper_greater_than,     80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss)");
            addOperator(L">=",      true, oper_greater_than_eq,  80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss)");
            addOperator(L"in",      true, oper_in,               80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"instanceof", true, oper_instanceof,    80, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"==",      true, oper_equals,           70, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"!=",      true, oper_not_equals,       70, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"===",     true, oper_strict_equals,    70, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"!==",     true, oper_not_strict_equals,70, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(**)");
            addOperator(L"&",       true, oper_bitwise_and,      65, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"i(ii)");
            addOperator(L"^",       true, oper_bitwise_xor,      64, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"i(ii)");
            addOperator(L"|",       true, oper_bitwise_or,       63, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"i(ii)");
            addOperator(L"&&",      true, oper_and,              60, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true,  L"*(**)");
            addOperator(L"||",      true, oper_or,               60, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true,  L"*(**)");
            addOperator(L"?",       true, NULL,                  50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"+=",      true, oper_pluseq,           50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"-=",      true, oper_minuseq,          50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"*=",      true, oper_multeq,           50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"/=",      true, oper_diveq,            50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"%=",      true, oper_modeq,            50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"&=",      true, oper_bitandeq,         50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"|=",      true, oper_bitoreq,          50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"^=",      true, oper_bitxoreq,         50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L"<<=",     true, oper_shleq,            50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"n(nn)");
            addOperator(L">>=",     true, oper_shreq,            50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"n(nn)");
            addOperator(L">>>=",    true, oper_shrzfeq,          50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"n(nn)");
            addOperator(L"=",       true, oper_assign,           50, ExprOperator::typeNormal, ExprOperator::directionLeftToRight, true, L"*(**)");
            addOperator(L",",       true, oper_comma,            40, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true, L"*(**)");
        
            m_bindings.reserve(200);

            addFunction(L"parseInt",   true, jsparseInt, false, L"", this);
            addFunction(L"parseFloat", true, jsparseFloat, false, L"", this);
            addFunction(L"isNaN",      true, js_isNaN,  false, L"", this);
            addFunction(L"isFinite",   true, js_isFinite,  false, L"", this);
            
            Value* eval_func = addFunction(L"eval",       true, ExprParser::jsEvalStatic, false, L"", this);
            eval_func->m_scope = envUseCallerScope;
            
            addFunction(L"escape", true, js_escape, false, L"", this);
            addFunction(L"encodeURI", true, js_encodeURI, false, L"", this);
            addFunction(L"encodeURIComponent", true, js_encodeURIComponent, false, L"", this);
            addFunction(L"unescape", true, js_unescape, false, L"", this);
            addFunction(L"decodeURI", true, js_decodeURI, false, L"", this);
            addFunction(L"decodeURIComponent", true, js_decodeURIComponent, false, L"", this);


            // bind JS classes
            
            Array::compiletimeBind(this);
            Boolean::compiletimeBind(this);
            Date::compiletimeBind(this);
            Function::compiletimeBind(this);
            Json::compiletimeBind(this);
            Object::compiletimeBind(this);
            Math::compiletimeBind(this);
            Math::bindStatic(this);
            Number::compiletimeBind(this);
            Number::bindStatic(this);
            String::compiletimeBind(this);
            RegExp::compiletimeBind(this);
            
            Error::compiletimeBind(this);
            TypeError::compiletimeBind(this);
            RangeError::compiletimeBind(this);
            SyntaxError::compiletimeBind(this);
            EvalError::compiletimeBind(this);
            URIError::compiletimeBind(this);
            ReferenceError::compiletimeBind(this);
            
            Value v;
            v.setUndefined();
            addValue(L"undefined", v)->setAttributes(Value::attrDontDelete);
            
            v.setNaN();
            addValue(L"NaN", v)->setAttributes(Value::attrDontDelete);
            
            v.setPositiveInfinity();
            addValue(L"Infinity", v)->setAttributes(Value::attrDontDelete);
        }
    }
     else if (m_language == optionLanguageGeneric)
    {
        if (!(flags & optionNoAutoBindings))
        {
            addOperator(L"(", L")", true, NULL,              130, ExprOperator::typeParenthesized, ExprOperator::directionRightToLeft, true, L"*(*i);*(*s)");
            addOperator(L"not", false, oper_not,             110, ExprOperator::typeUnary,  ExprOperator::directionRightToLeft, false, L"b(b)");
            addOperator(L"!",   true,  oper_not,             110, ExprOperator::typeUnary,  ExprOperator::directionRightToLeft, false, L"b(b)");
            addOperator(L"-",   true,  oper_negate,          110, ExprOperator::typeUnary,  ExprOperator::directionRightToLeft, false, L"f(n)");
            addOperator(L"%",   true,  oper_mod,             100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"mod", false, oper_mod,             100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"i(nn)");
            addOperator(L"/",   true,  oper_divide,          100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"*",   true,  oper_multiply,        100, ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn)");
            addOperator(L"+",   true,  oper_add,             90,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn);s(ss);d(dn);x(xx)");
            addOperator(L"||",  true,  oper_concat,          90,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"s(**)");
            addOperator(L"-",   true,  oper_subtract,        90,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"f(nn);d(dn);i(dd)");
            addOperator(L"<",   true,  oper_less_than,       80,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd)");
            addOperator(L"<=",  true,  oper_less_than_eq,    80,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd)");
            addOperator(L">",   true,  oper_greater_than,    80,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd)");
            addOperator(L">=",  true,  oper_greater_than_eq, 80,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd)");
            addOperator(L"!=",  true,  oper_not_equals,      70,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd);b(bb);b(xx)");
            addOperator(L"=",   true,  oper_equals,          70,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd);b(bb);b(xx)");
            addOperator(L"<>",  true,  oper_not_equals,      70,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(nn);b(ss);b(dd);b(bb);b(xx)");
            addOperator(L"like", false, oper_like,           70,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(*s)");
            addOperator(L"not like", false, oper_notlike,    70,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"b(*s)");
            addOperator(L"and", false, oper_and,             60,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true, L"b(bb)");
            addOperator(L"or",  false, oper_or,              60,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, true,  L"b(bb)");
            addOperator(L":=",  true,  oper_assign,          50,  ExprOperator::typeNormal, ExprOperator::directionRightToLeft, false, L"");
        }
    }
    
    GlobalObject* global_object = new GlobalObject;
    m_global_object.setObject(global_object);
}


ExprParser::~ExprParser()
{
    m_global_object.reset();
    m_object_prototype.getObject()->deleteAllMembers();
    m_function_prototype.getObject()->deleteAllMembers();

    if (m_entrypt)
        delete m_entrypt;

    std::vector<ExprVarBinding>::iterator binding_it;
    std::vector<ExprVarBinding>::iterator bind_begin = m_bindings.begin();
    std::vector<ExprVarBinding>::iterator bind_end = m_bindings.end();

    for (binding_it = bind_begin; binding_it != bind_end; ++binding_it)
        delete (binding_it->m_start_value);
    
    std::map<std::wstring,ExprClassInfo*>::const_iterator cit, cit_end;
    cit_end = m_classes.end();
    for (cit = m_classes.begin(); cit != cit_end; ++cit)
        delete (cit->second);
        
    std::vector<ExprEnv*>::iterator it;
    for (it = m_envs.begin(); it != m_envs.end(); ++it)
    {
        (*it)->m_parser = NULL;
        // the env will be personally deleted by us, this will
        // prevent unwanted reentrant calls to reset()
        (*it)->ref();
    }
    for (it = m_envs.begin(); it != m_envs.end(); ++it)
        (*it)->reset();
    for (it = m_envs.begin(); it != m_envs.end(); ++it)
        delete (*it);
        
    delete m_root_penv;
}



void ExprParser::setParseHook(int parse_hook_mask,
                              ExprParseHookFunc func,
                              void* param)
{
    if (func)
    {
        m_parse_hook_mask = parse_hook_mask;
        m_parse_hook = func;
        m_parse_hook_param = param;
    }
}



ExprClassInfo* ExprParser::addClass(const std::wstring& class_name,
                                    const std::wstring& baseclass_name,
                                    kscript::ShadowObject* prototype_shadow,
                                    ExprBindFunc initbind_ptr)
{
    ExprClassInfo* info = getClassInfo(class_name);
    ExprClassInfo* base_info = getClassInfo(baseclass_name);
    
    info->m_objinit_func = initbind_ptr;
    info->m_base_name = baseclass_name;
    info->m_prototype.getObject()->setShadowObject(prototype_shadow);
    
    if (base_info)
    {
        // make sure to chain prototypes together for derived classes
        info->m_prototype.setPrototype(&base_info->m_prototype);
    }
    
    
    // add constructor binding

    ExprVarBinding b;
    b.m_name = class_name;
    b.m_base_name = baseclass_name;
    b.m_formats = L"*(...)";
    b.m_case_sense = true;
    b.m_var_type = Value::typeFunction;
    
    b.m_start_value = new Value;
    m_bindings.push_back(b);
    info->m_class_obj = b.m_start_value;
    

    Function* def = createFunction(&info->m_prototype);
    def->m_func_ptr = (ExprBindFunc)kscript_object_constructor;
    def->m_binding_param = info;
    Value* val_prototype = def->getMember(L"prototype");
    val_prototype->setAttributes(Value::attrDontEnum | Value::attrDontDelete | Value::attrReadOnly);
    
    info->m_class_obj->setFunction(def);
    info->m_class_obj->getObject()->setShadowObject(prototype_shadow);
    
    
    kscript::Value v;
    if (prototype_shadow && prototype_shadow->getMember(L"constructor", &v))
    {
        // class already supplies a constructor, we don't need to use
        // the Function object's built-in one
        val_prototype->getObject()->deleteMember(L"constructor");
        
        kscript::Value* v = info->m_prototype.getMember(L"constructor");
        if (v && v->isFunction())
            info->m_cstdef = v->getFunction();      
    }
    
    return info;
}


Value* ExprParser::addFunction(const std::wstring& func_name,
                               bool case_sense,
                               ExprBindFunc func_ptr,
                               bool defer_calc,
                               const std::wstring& formats,
                               void* param)
{
    ExprVarBinding b;

    b.m_name = func_name;
    b.m_formats = formats;
    b.m_case_sense = case_sense;
    b.m_var_type = Value::typeFunction;

    Function* def = createFunction();
    def->m_func_ptr = func_ptr;
    def->m_binding_param = param;
    def->m_defer_calc = defer_calc;

    b.m_start_value = new Value;
    b.m_start_value->setFunction(def);

    m_bindings.push_back(b);
    
    return b.m_start_value;
}

Value* ExprParser::addVarBinding(const std::wstring& var_name,
                               bool case_sense,
                               int var_type,
                               void* ptr,
                               void* param)
{
    ExprVarBinding b;

    b.m_name = var_name;
    b.m_case_sense = case_sense;
    b.m_var_type = var_type;

    Value* v = new Value;
    v->m_type = var_type;
    v->m_method = Value::methodGetVal;
    v->m_getvalptr = (ExprBindFunc)ptr;
    v->m_getvalparam = param;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}

Value* ExprParser::addDoubleValue(const std::wstring& var_name,
                                  double value)
{
    Value* v = new Value;
    v->setDouble(value);

    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = Value::typeDouble;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}
                        
Value* ExprParser::addIntegerValue(const std::wstring& var_name,
                                   int value)
{
    Value* v = new Value;
    v->setInteger(value);

    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = Value::typeInteger;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}

Value* ExprParser::addStringValue(const std::wstring& var_name,
                                const std::wstring& value)
{
    Value* v = new Value;
    v->setString(value.c_str(), value.length());

    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = Value::typeInteger;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}

Value* ExprParser::addObjectValue(const std::wstring& var_name,
                                  ValueObject* obj)
{
    Value* v = new Value;
    v->setObject(obj);

    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = Value::typeObject;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}

Value* ExprParser::addNullValue(const std::wstring& var_name)
{
    Value* v = new Value;
    v->setNull();

    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = Value::typeInteger;
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}

Value* ExprParser::addValue(const std::wstring& var_name,
                            Value& val)
{
    Value* v = new Value;
    v->setValue(val);
    
    ExprVarBinding b;
    b.m_name = var_name;
    b.m_case_sense = true;
    b.m_var_type = v->getType();
    b.m_start_value = v;

    m_bindings.push_back(b);
    
    return v;
}
    
ExprOperator& ExprParser::addOperator(const std::wstring& oper_name,
                                      bool case_sense,
                                      ExprBindFunc func_ptr,
                                      int priority,
                                      int type,
                                      int direction,
                                      bool defer_calc,
                                      const std::wstring& formats)
{
    ExprOperator e;
    e.m_name = oper_name;
    e.m_formats = formats;
    e.m_func_ptr = func_ptr;
    e.m_priority = priority;
    e.m_type = type;
    e.m_direction = direction;
    e.m_defer_calc = defer_calc;
    e.m_alpha = true;
    e.m_case_sense = case_sense;

    const wchar_t* s = oper_name.c_str();
    while (*s)
    {
        if (!iswalpha(*s))
        {
            e.m_alpha = false;
            break;
        }
        s++;
    }

    m_opers.push_back(e);

    return m_opers.back();
}

ExprOperator& ExprParser::addOperator(const std::wstring& oper_open,
                                      const std::wstring& oper_close,
                                      bool case_sense,
                                      ExprBindFunc func_ptr,
                                      int priority,
                                      int type,
                                      int direction,
                                      bool defer_calc,
                                      const std::wstring& formats)
{
    ExprOperator& o = addOperator(oper_open,
                                  case_sense,
                                  func_ptr,
                                  priority,
                                  type,
                                  direction,
                                  defer_calc,
                                  formats);
    o.m_name_close = oper_close;
    return o;
}


void ExprParser::addUnresolved(ExprVarLookup* v)
{
    if (m_unresolved.find(v) != m_unresolved.end())
    {
        // already in map
        return;
    }
    
    m_unresolved.insert(v);
}


void ExprParser::removeUnresolved(ExprVarLookup* v)
{
    m_unresolved.erase(v);
}

bool ExprParser::resolveUnresolved()
{
    if (m_unresolved.size() == 0)
        return true;
        
    int error_code = 0;
    std::wstring error_text = L"";
    const wchar_t* error_loc = NULL;

    // try to resolve forward references to functions
    
    std::set<ExprVarLookup*>::iterator it, unresolved_end = m_unresolved.end();
    for (it = m_unresolved.begin(); it != unresolved_end; ++it)
    {
        // prevent ExprVarLookup objects from deregistering
        // themselves when they are destroyed
        (*it)->m_parser = NULL;
        
        ExprParserToken* token;
        token = (*it)->m_scope->lookupToken((*it)->m_symbol);
        if (token)
        {
            (*it)->m_mode = ExprVarLookup::modeVarId;
            (*it)->m_var_id = token->id;
        }
         else
        {
            // still unresolved, check if we are in a class
            if (!(m_flags & optionStrictClassScoping))
            {
                ExprClassInfo* class_info = (*it)->m_scope->getClassInfo();
                if (class_info)
                {
                    if (class_info->getMemberExistsInHierarchy((*it)->m_symbol))
                    {
                        // it's a class member, so it's ok
                        (*it)->m_mode = ExprVarLookup::modeObjectMember;
                        continue;
                    }
                }
            }

            if ((m_flags & optionForceVarDeclare))
            {
                if (error_code == 0 || (*it)->m_location <= error_loc)
                {
                    // always report the error that happened first
                    error_loc = (*it)->m_location;
                    error_code = errorUndeclaredIdentifier;
                    error_text = (*it)->m_symbol;
                }
            }
             else
            {
                // variable is still not found, look in the global this object
                (*it)->m_mode = ExprVarLookup::modeRuntime;
                (*it)->m_var_id = getSymbolId((*it)->m_symbol);
                continue;
            }
        }
    }
    
    if (error_code != 0)
    {
        m_error_loc = error_loc;
        m_error_code = error_code;
        m_error_text = error_text;
        return false;
    }
    
    
    m_unresolved.clear();
    return true;
}


Function* ExprParser::createFunction(Value* prototype)
{
    Function* f = Function::createObject(this);
        
    // set func.__proto__ (for Function and object methods)
    f->setPrototype(&m_function_prototype);
    
    // set func.prototype (copied to objects' __proto__ when "new" invoked)
    if (prototype)
    {
        f->createMember(L"prototype", 0)->setValue(prototype);
    }
     else
    {
        prototype = f->createMember(L"prototype", 0);
        prototype->setObject(Object::createObject(this));
    }

    // set func.prototype.constructor

    ClassConstructorRef* ref = new ClassConstructorRef;
    ref->m_function = f;
    prototype->createMember(L"constructor", Value::attrDontEnum)->setExprElement(ref);
    
    return f;
}


ExprElement* ExprParser::createVariableLookup(ExprParserEnv* penv, const std::wstring& identifier)
{
    ExprParserToken* var_token = penv->lookupToken(identifier);

    if (var_token)
    {
        ExprVarLookup* varlookup = new ExprVarLookup;
        varlookup->m_var_id = var_token->id;
        varlookup->m_symbol = identifier;
        if (var_token->binding)
            varlookup->m_retval_type = var_token->binding->m_var_type;

        return varlookup;
    }

    return NULL;
}

bool ExprParser::createObject(const std::wstring& class_name,
                              kscript::Value* obj)
{
    Value* cst = getBindVariable(class_name);
    if (!cst)
        return false;

    if (cst->getType() != Value::typeFunction)
        return false;

    obj->reset();

    Value retval;
    if (evalSucceeded != invoke(m_global_scope, cst, obj, &retval))
    {
        obj->setNull();
        return false;
    }

    if (obj->isNull())
    {
        return false;
    }

    return true;
}

void ExprParser::bindObject(ValueObject* obj)
{
    int class_id = obj->getClassId();
    if (class_id <= 0)
        return;
    
    obj->setParser(this);
    
    // first, get class information from our lookup vector
    ExprClassInfo* info = NULL;
    
    if (class_id >= (int)m_class_lookup.size())
        m_class_lookup.resize(class_id+20);

    info = m_class_lookup[class_id];
    if (!info)
    {
        info = getClassInfo(obj->getClassName());
        if (!info)
            return;
        m_class_lookup[class_id] = info;
    }
      

    info->setupObject(obj);
}



void ExprParser::jsEvalStatic(ExprEnv* env, void* param, Value* retval)
{
    env->getParser()->jsEval(env, param, retval);
}

void ExprParser::jsEval(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count == 0)
    {
        retval->reset();
        return;
    }
    
    if (!env->getParam(0)->isString())
    {
        env->getParam(0)->eval(env, retval);
        return;
    }
     else
    {
        // first, find the appropriate parser context
        ExprParserEnv* penv = env->getParserContext();
        
        // if there is no parser environment (this shouldn't happen),
        // we can't parse, so fail out
        if (!penv)
        {
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
    
        wchar_t* expr = env->m_eval_params[0]->getString();
        if (!removeComments(expr))
        {
            // missing close comment
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
        
        m_unresolved.clear();
            
        ExprSequence* e = (ExprSequence*)parseSequence(penv, expr, NULL, true);
        
        if (!e)
        {
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
        
        if (!resolveUnresolved())
        {
            env->setRuntimeError(rterrorSyntaxError);
            return;
        }
        
        e->m_always_wants_return = true;
        e->m_create_env = false;
                
        // do the eval
        ExprEnv* evalenv = createEnv();
        evalenv->setParent(env);
        evalenv->setThis(env->getThis());
        
        e->eval(evalenv, retval);
        
        evalenv->unref();
        
        // delete the sequence's expr parser env
        e->m_penv->m_parent->removeChild(e->m_penv);
        delete e->m_penv;
        
        // delete the sequence
        delete e;
    }
}





Value* ExprParser::getBindVariable(const std::wstring& bind_name)
{
    std::vector<ExprVarBinding>::iterator it;

    int var_id = -1;

    std::vector<ExprVarBinding>::iterator bind_begin = m_bindings.begin();
    std::vector<ExprVarBinding>::iterator bind_end = m_bindings.end();

    for (it = bind_begin; it != bind_end; ++it)
    {
        if (it->m_case_sense)
        {
            if (it->m_name == bind_name)
            {
                var_id = it->m_var_id;
                if (var_id == -1)
                {
                    if (!it->m_start_value)
                        it->m_start_value = new Value;
                    return it->m_start_value;
                }
                break;
            }
        }
         else
        {
            if (!wcscasecmp(it->m_name.c_str(), bind_name.c_str()))
            {
                var_id = it->m_var_id;
                if (var_id == -1)
                {
                    if (!it->m_start_value)
                        it->m_start_value = new Value;
                    return it->m_start_value;
                }
                break;
            }
        }
    }

    if (var_id != -1)
    {
        return m_global_scope->getValue(var_id);
    }

    return NULL;
}



void ExprParser::calcErrorInfo()
{
    wchar_t* expr = NULL;
    size_t offset = 0;

    m_error_line = 0;
    m_error_file = L"";
    m_error_offset = 0;

    // figure out which file the error happened in
    size_t last_offset = 0xffffffff;
    std::vector<ExprParserSource>::iterator src_it;
    for (src_it = m_sources.begin(); src_it != m_sources.end(); ++src_it)
    {
        size_t off = m_error_loc - src_it->m_src;
        if (off < last_offset)
        {
            last_offset = off;
            
            expr = src_it->m_src;
            offset = m_error_loc - src_it->m_src;
            m_error_file = src_it->m_file;
        }
    }
    
    if (!expr)
        return;
    
    if (m_error_code != errorNone)
    {
        // error occurred, find line number
        int line = 0;
        wchar_t* e = expr;
        while (*e)
        {
            if (*e == 0x0a)
                line++;

            e++;

            if (m_error_loc)
            {
                if (e == m_error_loc)
                    break;
            }
        }

        m_error_line = line;
    }
     else
    {
        // an unknown error happened
        m_error_code = errorUnknown;
    }

    if (m_error_loc)
        m_error_offset = m_error_loc - expr;
         else
        m_error_offset = wcslen(expr);
}


bool ExprParser::parse(const std::wstring& _expr)
{
    m_cancelled = false;

    m_error_code = errorNone;
    m_error_line = 0;
    m_error_loc = NULL;
    m_error_text = L"";


    // this will be freed later on in the for
    // loop at the end; look for "free all string buffers" below
    size_t bytes = sizeof(wchar_t)*(_expr.length()+1);
    wchar_t* expr = (wchar_t*)malloc(bytes);
    if (!expr)
    {
        m_error_loc = NULL;
        m_error_code = errorOutOfMemory;
        m_error_text = L"";
        return false;
    }

    memcpy(expr, _expr.c_str(), bytes);
    
    
    ExprParserSource parse_src;
    parse_src.m_src = expr;
    parse_src.m_file = L"";
    m_sources.push_back(parse_src);
    
    
    if (m_language == optionLanguageECMAScript)
    {
        if (!removeComments(expr))
        {
            free(expr);
            m_error_loc = NULL;
            m_error_code = errorUnterminatedComment;
            m_error_text = L"";
            return false;
        }
    }
     else
    {
        wchar_t* e = expr;
        while (iswspace(*e))
            e++;
        
        // an empty generic expression is a bad one
        if (!*e)
        {
            free(expr);
            return false;
        }
    }
    
    
    m_root_penv = new ExprParserEnv(NULL);
    m_root_penv->m_parser = this;


    m_root_penv->reserve(m_bindings.size());
    
    // add bindings to the parser's environment
    std::vector<ExprVarBinding>::iterator it, bindings_end;
    bindings_end = m_bindings.end();
    for (it = m_bindings.begin(); it != bindings_end; ++it)
    {
        it->m_var_id = m_root_penv->declareToken(it->m_name,
                                                 it->m_case_sense,
                                                 &(*it));
    }




    // parse the expression

    m_entrypt = parseSequence(m_root_penv, expr);


    if (m_entrypt)
    {
        // top sequence should use global scope environment
        
        m_entrypt->m_create_env = false;
        
        if (!resolveUnresolved())
        {
            delete m_entrypt;
            m_entrypt = NULL;
        }
    }

    if (m_entrypt == NULL)
    {
        calcErrorInfo();
    }

    // free all string buffers
    std::vector<ExprParserSource>::iterator src_it;
    for (src_it = m_sources.begin(); src_it != m_sources.end(); ++src_it)
        free(src_it->m_src);
    
    

    if (m_entrypt == NULL)
        return false;
    
    
    m_retval_type = m_entrypt->getType();

    if (m_language == kscript::optionLanguageGeneric && m_retval_type == Value::typeNull)
    {
        delete m_entrypt;
        m_entrypt = NULL;
        return false;
    }

    return true;
}

void ExprParser::setMaximumDepth(int max_depth)
{
    m_max_depth = max_depth;
}

void ExprParser::setExtraLong(long val)
{
    m_extra_long = val;
}

long ExprParser::getExtraLong()
{
    return m_extra_long;
}
    

ExprErrorInfo ExprParser::getErrorInfo()
{
    ExprErrorInfo info;
    info.m_code = m_error_code;
    info.m_line = m_error_line;
    info.m_offset = m_error_offset;
    info.m_text = m_error_text;
    info.m_filename = m_error_file;
    return info;
}

void ExprParser::setRuntimeError(int rterror_code,
                                 Value* exception_obj,
                                 const std::wstring& text)
{
    m_error_code = rterror_code;
    m_error_text = text;
    
    if (m_error_code == rterrorNone)
        return;
        
    if (exception_obj)
    {
        m_exception_obj.setValue(exception_obj);
    }
     else
    {
        Error* e = NULL;
        
        switch (rterror_code)
        {
            default:
            case rterrorTypeError:
                e = TypeError::createObject(this);
                break;
            case rterrorSyntaxError:
                e = SyntaxError::createObject(this);
                break;
            case rterrorRangeError:
                e = RangeError::createObject(this);
                break;
            case rterrorReferenceError:
                e = ReferenceError::createObject(this);
                break;
        }
            
        m_exception_obj.setObject(e);
    }
}

ExprEnv* ExprParser::createEnv()
{
    ExprEnv* env;
    
    if (m_unused_envs.size() > 0)
    {
        env = m_unused_envs.top();
        m_unused_envs.pop();
        env->ref();
        return env;
    }
    
    env = new ExprEnv;
    env->m_parser = this;
    m_envs.push_back(env);
    
    env->ref();
    return env;
}

void ExprParser::returnEnv(ExprEnv* env)
{
    m_unused_envs.push(env);
}


bool ExprParser::eval(Value* retval)
{
    if (!m_entrypt)
    {
        m_error_code = rterrorNoEntryPoint;
        return false;
    }

    if (!m_bindings_defined)
    {
        m_bindings_defined = true;

        m_global_scope = createEnv();
        m_global_scope->setParent(NULL);
        m_global_scope->setThis(getGlobalObject());

        Value* v;
        
        std::vector<ExprVarBinding>::iterator it;
        std::vector<ExprVarBinding>::iterator bind_begin = m_bindings.begin();
        std::vector<ExprVarBinding>::iterator bind_end = m_bindings.end();

        // let global object know about global scope
        ((GlobalObject*)m_global_object.getObject())->initGlobalObject(this, m_global_scope);

        // this just makes the declareVar() run a bit faster
        m_global_scope->reserveVars(m_bindings.size()+10);
        
        // go through each of the bound variables and declare
        // each one in the global scope
        for (it = bind_begin; it != bind_end; ++it)
        {
            v = m_global_scope->declareVar(it->m_var_id);

            if (it->m_start_value)
            {
                v->copyFrom(it->m_start_value);
            }
        }
    }

    int result = m_entrypt->eval(m_global_scope, retval);
    
    if (retval)
    {
        // don't let referenced scope objects escape
        retval->releaseScope();
    }
    
    return succeeded(result);
}

int ExprParser::invoke(ExprEnv* env,
                       Value* func,
                       Value* vthis,
                       Value* retval,
                       Value** vparams,
                       size_t param_count)
{
    if (func->getType() != Value::typeFunction)
        return evalFailed;

    Function* func_info = func->getFunction();
    return func_info->invoke(env, vthis, retval, vparams, param_count);
}

ExprClassInfo* ExprParser::getClassInfo(const std::wstring& class_name)
{
    if (class_name.length() == 0)
        return NULL;
        
    std::map<std::wstring,ExprClassInfo*>::const_iterator it;
    it = m_classes.find(class_name);
    if (it != m_classes.end())
        return it->second;
        

    ExprClassInfo* ci = new ExprClassInfo(this);
    ci->m_name = class_name;
    ci->m_cstdef = NULL;
    m_classes[class_name] = ci;


    return ci;
}


ExprClassInfo* ExprParser::getClassInfo(Value* value)
{
    if (value->isFunction())
    {
        Function* func = value->getFunction();
        if (func->m_func_ptr != kscript_object_constructor ||
            func->m_binding_param == NULL)
        {
            return NULL;
        }
        
        return (ExprClassInfo*)func->m_binding_param;
    }
    
    if (value->isObject())
    {
        return getClassInfo(value->getObject()->getClassName());
    }
    
    return NULL;
}


bool ExprParser::getClassExist(const std::wstring& class_name)
{
    if (m_classes.find(class_name) != m_classes.end())
        return true;

    return false;
}


int ExprParser::allocateClassId()
{
    static int id = 0;
    return ++id;
}


void ExprParser::cancel()
{
    m_cancelled = true;
}

int ExprParser::getType()
{
    return m_retval_type;
}

};

