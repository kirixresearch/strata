/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-11-02
 *
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "kscript.h"
#include "util.h"
#include "jsfunction.h"
#include "functions.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <locale>


#ifdef _MSC_VER
#include <cfloat>
#define isnan _isnan
#define swprintf _snwprintf
#define wcscasecmp wcsicmp
#endif


#define DBLCOMPARE(x,y) env->m_parser->dblcompare((x),(y))


namespace kscript
{


// date comparison helper function

// < 0 date1 less than date2 
// 0 date1 identical to date2 
// > 0 date1 greater than date2 

inline int dtCompare(const ExprDateTime* d1, const ExprDateTime* d2)
{
    if (d1->date < d2->date)
        return -1;
    if (d1->date == d2->date)
    {
        if (d1->time < d2->time)
            return -1;
        if (d1->time == d2->time)
            return 0;
    }
    return 1;
}

void oper_array(ExprEnv* env, void* param, Value* retval)
{
    ExprOperArrayInfo* arrinfo = env->m_valreturn_arrinfo;
    env->m_valreturn_arrinfo = NULL;
    
    if (env->m_valreturn_this)
    {
        Value right;
        Value& left = *env->m_valreturn_this;
        env->m_valreturn_this = NULL;
        
        env->m_params[0]->eval(env, &left);
        env->m_params[1]->eval(env, &right);

        if (arrinfo)
        {
            arrinfo->left.setValue(left);
            arrinfo->right = right.getString();
            if (arrinfo->noeval)
            {
                retval->reset();
                return;
            }
            retval->setRef(left.getMember(arrinfo->right));
        }
         else
        {
            retval->setRef(left.getMember(right.getString()));
        }
    }
     else
    {
        Value* left = env->m_params[0]->getLvalue(env);
        
        if (left && !left->isExprElement())
        {
            Value right;
            env->m_params[1]->eval(env, &right);
            
            if (arrinfo)
            {
                arrinfo->left.setValue(left);
                arrinfo->right = right.getString();
                if (arrinfo->noeval)
                {
                    retval->reset();
                    return;
                }
                retval->setRef(left->getMember(arrinfo->right));
            }
             else
            {
                retval->setRef(left->getMember(right.getString()));
            }
        }
          else
        {
            Value left, right;
            env->m_params[0]->eval(env, &left);
            env->m_params[1]->eval(env, &right);
            
            if (arrinfo)
            {
                arrinfo->left.setValue(left);
                arrinfo->right = right.getString();
                if (arrinfo->noeval)
                {
                    retval->reset();
                    return;
                }
            }
            
            if (left.isObject() && left.getObject()->m_base_ref_count > 1)
            {
                retval->setRef(left.getMember(right.getString()));
            }
             else
            {
                // we can't return a reference, because |left| is either not
                // an object or is a transient one (ref count is currently 1)
                retval->setValue(left.getMember(right.getString()));
            }
        }
    }
}


// bitwise operators

void oper_bitwise_and(ExprEnv* env, void* param, Value* retval)
{
    retval->setInteger(env->m_eval_params[0]->getInteger() &
                       env->m_eval_params[1]->getInteger());
}

void oper_bitwise_xor(ExprEnv* env, void* param, Value* retval)
{
    retval->setInteger(env->m_eval_params[0]->getInteger() ^
                       env->m_eval_params[1]->getInteger());
}

void oper_bitwise_or(ExprEnv* env, void* param, Value* retval)
{
    retval->setInteger(env->m_eval_params[0]->getInteger() |
                       env->m_eval_params[1]->getInteger());
}

void oper_bitwise_not(ExprEnv* env, void* param, Value* retval)
{
    retval->setInteger(~env->m_eval_params[0]->getInteger());
}

void oper_shift_left(ExprEnv* env, void* param, Value* retval)
{
    int p0 = env->m_eval_params[0]->getInteger();
    int p1 = ((unsigned int)env->m_eval_params[1]->getInteger()) & 0x1f;
    retval->setInteger(p0 << p1);
}

void oper_shift_right(ExprEnv* env, void* param, Value* retval)
{
    int p0 = env->m_eval_params[0]->getInteger();
    int p1 = ((unsigned int)env->m_eval_params[1]->getInteger()) & 0x1f;
    retval->setInteger(p0 >> p1);
}

void oper_shift_right_zf(ExprEnv* env, void* param, Value* retval)
{
    unsigned int p0 = (unsigned int)env->m_eval_params[0]->getInteger();
    unsigned int p1 = ((unsigned int)env->m_eval_params[1]->getInteger()) & 0x1f;
    retval->setDouble((double)(p0 >> p1));
}


// logical operators

void oper_negate(ExprEnv* env, void* param, Value* retval)
{
    if (!env->m_eval_params[0]->isNumber())
    {
        env->m_eval_params[0]->toNumber(env->m_eval_params[0]);
    }


    // unary negation operator
    if (env->m_eval_params[0]->getType() == Value::typeInteger)
    {
        int i = env->m_eval_params[0]->getInteger();
        if (i == 0)
            retval->setDouble(-0.0);
             else
            retval->setInteger(-i);
    }
     else
    {
        if (env->m_eval_params[0]->isNull())
            retval->setNaN();
             else
            retval->setDouble(-env->m_eval_params[0]->getDouble());
    }
}

void oper_unary_plus(ExprEnv* env, void* param, Value* retval)
{
    if (!env->m_eval_params[0]->isNumber())
    {
        env->m_eval_params[0]->toNumber(retval);
        return;
    }
    
    if (env->m_eval_params[0]->getType() == Value::typeInteger)
    {
        retval->setInteger(env->m_eval_params[0]->getInteger());
    }
     else
    {
        retval->setDouble(env->m_eval_params[0]->getDouble());
    }
}

void oper_not(ExprEnv* env, void* param, Value* retval)
{
    // unary boolean reversing operator

    retval->setBoolean(!env->m_eval_params[0]->getBoolean());
}

void oper_and(ExprEnv* env, void* param, Value* retval)
{
    env->m_params[0]->eval(env, retval);
    if (!retval->getBoolean())
        return;
    
    env->m_params[1]->eval(env, retval);
}


void oper_or(ExprEnv* env, void* param, Value* retval)
{
    env->m_params[0]->eval(env, retval);
    if (retval->getBoolean())
        return;
        
    env->m_params[1]->eval(env, retval);
}


void oper_increment(ExprEnv* env, void* param, Value* retval)
{
    Value* p1 = env->m_params[0]->getLvalue(env);
    if (p1 == NULL || (ExprElement*)p1 == env->m_params[0])
    {
        env->setRuntimeError(rterrorReferenceError);
        return;
    }


    if (p1->getType() == Value::typeDouble)
    {
        if (!env->m_prefix)
        {
            p1->eval(env, retval);
        }

        p1->setDouble(p1->getDouble() + 1.0);

        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
    }
     else if (p1->getType() == Value::typeInteger)
    {
        if (!env->m_prefix)
        {
            p1->eval(env, retval);
        }

        p1->setInteger(p1->getInteger() + 1);

        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
    }
     else
    {
        kscript::Value val;
        p1->toNumber(&val);
        val.setDouble(p1->getDouble());
        
        if (!env->m_prefix)
        {
            val.eval(env, retval);
        }
        
        p1->setDouble(p1->getDouble() + 1.0);
        
        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
        
    /*
        kscript::Value val1;
        p1->eval(env, &val1);
        val1.toNumber(retval);
        if (retval->isDouble())
            retval->setDouble(retval->getDouble() + 1.0);
        else if (retval->isInteger())
            retval->setInteger(retval->getInteger() + 1);
        else retval->setNull();
    */
    }
}

void oper_decrement(ExprEnv* env, void* param, Value* retval)
{
    Value* p1 = env->m_params[0]->getLvalue(env);
    if (p1 == NULL || (ExprElement*)p1 == env->m_params[0])
    {
        env->setRuntimeError(rterrorReferenceError);
        return;
    }

    if (p1->getType() == Value::typeDouble)
    {
        if (!env->m_prefix)
        {
            p1->eval(env, retval);
        }
        p1->setDouble(p1->getDouble() - 1.0);
        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
    }
     else if (p1->getType() == Value::typeInteger)
    {
        if (!env->m_prefix)
        {
            p1->eval(env, retval);
        }
        p1->setInteger(p1->getInteger() - 1);
        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
    }
     else
    {
        kscript::Value val;
        p1->toNumber(&val);
        val.setDouble(p1->getDouble());
        
        if (!env->m_prefix)
        {
            val.eval(env, retval);
        }
        
        p1->setDouble(p1->getDouble() - 1.0);
        
        if (env->m_prefix)
        {
            p1->eval(env, retval);
        }
    }
}

void oper_assign(ExprEnv* env, void* param, Value* retval)
{
    ExprOperArrayInfo arrinfo;
    env->m_valreturn_arrinfo = &arrinfo;
    
    ExprElement* right = env->m_params[1];
    Value* left = env->m_params[0]->getLvalue(env);
    env->m_valreturn_arrinfo = NULL;
    
    if (left == NULL || (ExprElement*)left == env->m_params[0])
    {
        env->setRuntimeError(rterrorReferenceError);
        return;
    }
    
    if (left == right)
    {
        left->eval(env, retval);
        return;
    }

    // put right side in the return value
    right->eval(env, retval);
    
    if (arrinfo.left.isObject())
    {
        // if the left is an object, we set the value of the member
        // with the setMember() method.  This allows the object to
        // intercept this assignment operation, if desired
        
        arrinfo.left.getObject()->setMember(arrinfo.right, retval);
    }
     else
    {
        // simple variable assignment
        if (!left->getReadOnly())
            retval->eval(env, left);
    }
}

void oper_pluseq(ExprEnv* env, void* param, Value* retval)
{
    // this function is optimized for speed, which is why
    // the flow is a little different than one would expect
    
    Value* left_lv = env->m_params[0]->getLvalue(env);
    Value* left = left_lv;
    Value* right = env->m_params[1]->getLvalue(NULL);
    Value* right_bucket = env->m_eval_params[1];

    if (left == NULL || (ExprElement*)left == env->m_params[0])
    {
        env->setRuntimeError(rterrorReferenceError);
        return;
    }
    
    if (right == NULL)
    {
        // there's a more complex element on the right side;
        // we need to evaluate the left element into its bucket
        // because 'left' might be changed when evaluating the
        // right expression.  See an expression like
        // f += f |= e
    
        env->m_params[0]->eval(env, env->m_eval_params[0]);
        left = env->m_eval_params[0];
        
        env->m_params[1]->eval(env, right_bucket);
        right = right_bucket;
    }
    
    
    left->toPrimitive(left);
    
    if (right->isObject())
    {
        right->toPrimitive(right_bucket);
        right = right_bucket;
    }
    

    if (left->isString() || right->isString())
    {
        left->toString(left);
        if (!right->isString())
        {
            right->toString(right_bucket);
            right = right_bucket;
        }
        
        left->appendString(right->getString(), right->getDataLen());
        left->eval(env, retval);
    }
     else
    {
        left->toNumber(left);
        
        if (!right->isNumber())
        {
            right->toNumber(right_bucket);
            right = right_bucket;
        }
                  
        left->setDouble(left->getDouble() + right->getDouble());
        left->eval(env, retval);
    }
    
    if (left_lv != left)
    {
        left->eval(env, left_lv);
    }
}



static void x_eq(ExprEnv* env, void* param, Value* retval, ExprBindFunc func)
{
    Value* left = env->m_params[0]->getLvalue(env);
    if (left == NULL || (ExprElement*)left == env->m_params[0])
    {
        env->setRuntimeError(rterrorReferenceError);
        return;
    }
    
    Value lv;
    Value rv;
    env->m_params[0]->eval(env, &lv);
    env->m_params[1]->eval(env, &rv);



    Value** oldv = env->m_eval_params;
    Value* v[2];
    v[0] = &lv;
    v[1] = &rv;

    env->m_eval_params = v;

    func(env, param, retval);
    retval->eval(NULL, left);

    env->m_eval_params = oldv;
}

void oper_minuseq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_subtract);
}

void oper_multeq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_multiply);
}

void oper_diveq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_divide);
}

void oper_modeq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_mod);
}

void oper_bitandeq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_bitwise_and);
}

void oper_bitoreq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_bitwise_or);
}

void oper_bitxoreq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_bitwise_xor);
}

void oper_shleq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_shift_left);
}

void oper_shreq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_shift_right);
}

void oper_shrzfeq(ExprEnv* env, void* param, Value* retval)
{
    x_eq(env, param, retval, oper_shift_right_zf);
}


void oper_not_equals(ExprEnv* env, void* param, Value* retval)
{
    oper_equals(env, NULL, retval);
    retval->setBoolean(!retval->getBoolean());
}

void oper_equals(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];
    int left_type = left->getType();
    int right_type = right->getType();
        
    if (left_type == right_type ||
        (left->isNumber() && right->isNumber()))
    {
        if (left_type == Value::typeUndefined)
        {
            retval->setBoolean(true);
            return;
        }
        
        if (left_type == Value::typeNull)
        {
            retval->setBoolean(true);
            return;
        }
        
        if (left->isNumber())
        {
            if (left->isNaN() || right->isNaN())
            {
                retval->setBoolean(false);
                return;
            }
            
            if (left_type == Value::typeInteger && left_type == right_type)
            {
                if (left->getInteger() == right->getInteger())
                {
                    retval->setBoolean(true);
                    return;
                }
            }
             else
            {                
                if (0 == DBLCOMPARE(left->getDouble(), right->getDouble()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
            
            // +/-0 comparison goes here; see ECMA-262 section 11.9.3
            
            retval->setBoolean(false);
            return;
        }
         else
        {
            if (left_type == Value::typeString)
            {
                if (left->getDataLen() != right->getDataLen())
                {
                    // if strings are different length, don't bother comparing
                    retval->setBoolean(false);
                    return;
                }
                
                if (0 == wcscmp(left->getString(), right->getString()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
             else if (left_type == Value::typeBoolean)
            {
                if (left->getBoolean() == right->getBoolean())
                    retval->setBoolean(true);
                     else
                    retval->setBoolean(false);
                return;
            }
             else if (left_type == Value::typeFunction)
            {
                if (left->getFunction() == right->getFunction())
                    retval->setBoolean(true);
                     else
                    retval->setBoolean(false);
                return;
            }
             else if (left_type == Value::typeDateTime)
            {
                ExprDateTime left_dt = left->getDateTime();
                ExprDateTime right_dt = right->getDateTime();
                
                if (left_dt.date == right_dt.date && left_dt.time == right_dt.time)
                    retval->setBoolean(true);
                     else
                    retval->setBoolean(false);
                return;
            }
            
            
            if (left == right || (left->hasObject() && right->hasObject() && left->getObject() == right->getObject()))
                retval->setBoolean(true);
                 else
                retval->setBoolean(false);
            return;
        }
    }
     else
    {
        if ((left_type == Value::typeNull && right_type == Value::typeUndefined) ||
            (left_type == Value::typeUndefined && right_type == Value::typeNull))
        {
            retval->setBoolean(true);
            return;
        }
        
        if ((left->isNumber() && right->isString()) ||
            (left->isString() && right->isNumber()))
        {
            if (0 == DBLCOMPARE(left->getDouble(), right->getDouble()))
                retval->setBoolean(true);
                 else
                retval->setBoolean(false);
            return;
        }

        if (left_type == Value::typeBoolean ||
            right_type == Value::typeBoolean)
        {
            if (left->isNull() || right->isNull())
            {
                retval->setBoolean(false);
                return;
            }
            
            Value l, r;
            left->toNumber(&l);
            right->toNumber(&r);
            
            if (0 == DBLCOMPARE(l.getDouble(), r.getDouble()))
                retval->setBoolean(true);
                 else
                retval->setBoolean(false);
            return;
        }
    
        if (right_type == Value::typeObject &&
            (left->isString() || left->isNumber()))
        {
            Value conv;
            if (left->isString())
            {
                right->toString(&conv);
                if (0 == wcscmp(left->getString(), conv.getString()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
             else
            {
                right->toNumber(&conv);
                if (0 == DBLCOMPARE(left->getDouble(), conv.getDouble()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
        }
         else if (left_type == Value::typeObject &&
                  (right->isString() || right->isNumber()))
        {
            Value conv;
            if (right->isString())
            {
                left->toString(&conv);
                if (0 == wcscmp(conv.getString(), right->getString()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
             else
            {
                left->toNumber(&conv);
                if (0 == DBLCOMPARE(conv.getDouble(), right->getDouble()))
                {
                    retval->setBoolean(true);
                    return;
                }
            }
        }
    }
    
    retval->setBoolean(false);
}



inline void abstract_less_than(ExprEnv* env, Value* left, Value* right, Value* retval)
{
    left->toPrimitive(left, Value::typeDouble);
    right->toPrimitive(right, Value::typeDouble);
    
    if (left->isString() && right->isString())
    {
        bool result = (wcscmp(left->getString(), right->getString()) < 0) ? true:false;
        retval->setBoolean(result);
    }
     else if (left->isDateTime() && right->isDateTime())
    {
        ExprDateTime d1, d2;
        left->getDateTime(&d1);
        right->getDateTime(&d2);
        retval->setBoolean(dtCompare(&d1, &d2) < 0 ? true : false);
    }
     else
    {
        left->toNumber(left);
        right->toNumber(right);
        
        if (left->isNaN() || right->isNaN())
        {
            retval->setUndefined();
            return;
        }
        
        int cmp = DBLCOMPARE(left->getDouble(), right->getDouble());
        
        if (cmp == 0)
        {
            retval->setBoolean(false);
            return;
        }
        
        // missing, -0, +0, -Inf, +Inf
        
        retval->setBoolean(cmp < 0 ? true : false);
    }
}



void oper_less_than(ExprEnv* env, void* param, Value* retval)
{
    abstract_less_than(env, env->m_eval_params[0], env->m_eval_params[1], retval);
    
    if (retval->isUndefined())
        retval->setBoolean(false);
}

void oper_greater_than(ExprEnv* env, void* param, Value* retval)
{
    abstract_less_than(env, env->m_eval_params[1], env->m_eval_params[0], retval);
    
    if (retval->isUndefined())
        retval->setBoolean(false);
}

void oper_less_than_eq(ExprEnv* env, void* param, Value* retval)
{
    abstract_less_than(env, env->m_eval_params[1], env->m_eval_params[0], retval);
    
    if (retval->getBoolean() || retval->isUndefined())
        retval->setBoolean(false);
         else
        retval->setBoolean(true);
}

void oper_greater_than_eq(ExprEnv* env, void* param, Value* retval)
{
    abstract_less_than(env, env->m_eval_params[0], env->m_eval_params[1], retval);
    
    if (retval->getBoolean() || retval->isUndefined())
        retval->setBoolean(false);
         else
        retval->setBoolean(true);
}


void oper_mod(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    left->toNumber(left);
    right->toNumber(right);
    
    if (left->isNaN() || right->isNaN() || left->isAnyInfinity())
    {
        retval->setNaN();
        return;
    }
    
    if (right->isAnyInfinity())
    {
        retval->setDouble(left->getDouble());
        return;
    }
    
    retval->setDouble(fmod(left->getDouble(), right->getDouble()));
}

void oper_divide(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    left->toNumber(left);
    right->toNumber(right);
    
    if (left->isNaN() || right->isNaN())
    {
        retval->setNaN();
        return;
    }
    
    if (left->isAnyInfinity())
    {
        if (right->isAnyInfinity())
        {
            retval->setNaN();
            return;
        }
        
        if (left->getSign() == right->getSign())
            retval->setPositiveInfinity();
             else
            retval->setNegativeInfinity();
        return;
    }
    
    if (right->isAnyZero())
    {
        if (left->isAnyZero())
        {
            retval->setNaN();
            return;
        }
        
                
        if (left->getSign() == right->getSign())
            retval->setPositiveInfinity();
             else
            retval->setNegativeInfinity();
        return;
    }
    
    
    retval->setDouble(left->getDouble() / right->getDouble());
}

void oper_multiply(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    left->toNumber(left);
    right->toNumber(right);
    
    if (left->isNaN() || right->isNaN())
    {
        retval->setNaN();
        return;
    }

    retval->setDouble(left->getDouble() * right->getDouble());
}


void oper_add(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    left->toPrimitive(left);
    right->toPrimitive(right);

    if (left->isBinary())
    {
        int result_len = left->getDataLen() + right->getDataLen();
        retval->setType(Value::typeBinary);
        retval->allocMem(result_len);
        retval->setDataLen(result_len);
        unsigned char* result = retval->getBinary();
        memcpy(result, left->getBinary(), left->getDataLen());
        memcpy(result+left->getDataLen(), right->getBinary(), right->getDataLen());
        return;
    }

    if (left->isString() || right->isString())
    {
        // because one of the parameters is a string, the
        // output will be a string
        
        if (left->isString() && right->isString())
        {
            // string concatenation
            int left_len = left->getDataLen();
            int right_len = right->getDataLen();
            retval->setStringLen(left_len + right_len);
            wchar_t* result = retval->getString();
            memcpy(result, left->getString(), left_len*sizeof(wchar_t));
            memcpy(result+left_len, right->getString(), right_len*sizeof(wchar_t));
            *(result+left_len+right_len) = 0;
            return;
        }
        
        if (left->isString())
        {
            retval->setString(left->getString(), left->getDataLen());
            Value v;
            right->toString(&v);
            retval->appendString(v.getString(), v.getDataLen());
        }
         else
        {
            left->toString(retval);
            retval->appendString(right->getString(), right->getDataLen());
        }
        
        return;
    }

    
    if (left->isDateTime())
    {
        if (right->isNumber())
        {
            ExprDateTime dt;
            left->getDateTime(&dt);
            dt.date += right->getInteger();
            retval->setDateTime(dt.date, dt.time);
        }
         else
        {
            right->eval(env, retval);
        }
        return;
    }
    
    if (right->isDateTime())
    {
        if (left->isNumber())
        {
            ExprDateTime dt;
            right->getDateTime(&dt);
            dt.date += left->getInteger();
            retval->setDateTime(dt.date, dt.time);
        }
         else
        {
            right->eval(env, retval);
        }
        return;
    }
    
    retval->setDouble(left->getDouble() + right->getDouble());
}



void oper_subtract(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    if (left->getType() == Value::typeDateTime)
    {
        if (right->isNumber())
        {
            ExprDateTime dt;
            left->getDateTime(&dt);
            dt.date -= right->getInteger();
            retval->setDateTime(dt.date, dt.time);
        }
         else if (right->getType() == Value::typeDateTime)
        {
            ExprDateTime d1, d2;
            left->getDateTime(&d1);
            right->getDateTime(&d2);
            retval->setInteger(d1.date - d2.date);
        }
        
        return;
    }


    left->toNumber(left);
    right->toNumber(right);
    
    retval->setDouble(left->getDouble() - right->getDouble());
}


// Javascript functions/operators


void oper_strict_equals(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];
    
    if (!((left->getType() == right->getType()) ||
          (left->isString() && right->isString()) ||
          (left->isNumber() && right->isNumber())))
    {
        retval->setBoolean(false);
        return;
    }
    
    if (left->isNull() || left->isUndefined())
    {
        retval->setBoolean(true);
        return;
    }
    
        
    if (left->isNumber())
    {
        if (left->isNaN() || right->isNaN())
        {
            retval->setBoolean(false);
            return;
        }
        
        if (0 == DBLCOMPARE(left->getDouble(), right->getDouble()))
        {
            retval->setBoolean(true);
        }
         else
        {
            retval->setBoolean(false);
        }
                
        return;
    }
     else
    {
        if (left->isString())
        {
            if (left->getDataLen() == right->getDataLen() &&
                0 == wcscmp(left->getString(), right->getString()))
            {
                retval->setBoolean(true);
            }
             else
            {
                retval->setBoolean(false);
            }
            return;
        }
         else if (left->isBoolean())
        {
            if (left->getBoolean() == right->getBoolean())
                retval->setBoolean(true);
                 else
                retval->setBoolean(false);
            return;
        }
         else if (left->isObject())
        {
            if (left->getObject() == right->getObject())
                retval->setBoolean(true);
                 else
                retval->setBoolean(false);
            return;
        }
         else
        {
            retval->setBoolean(false);
            return;
        }
    }
    
    retval->setBoolean(false);
    return;
}

void oper_not_strict_equals(ExprEnv* env, void* param, Value* retval)
{
    oper_strict_equals(env, NULL, retval);
    retval->setBoolean(!retval->getBoolean());
}


void oper_in(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    if (!right->isObject())
    {
        env->setRuntimeError(kscript::rterrorTypeError);
        return;
    }

    const wchar_t* member_name = left->getString();
    
    retval->setBoolean(right->getMemberExists(member_name));
}


void oper_instanceof(ExprEnv* env, void* param, Value* retval)
{
    Value* left = env->m_eval_params[0];
    Value* right = env->m_eval_params[1];

    kscript::ExprClassInfo* info = env->getParser()->getClassInfo(right);
    if (!info)
    {
        env->setRuntimeError(kscript::rterrorTypeError);
        return;
    }
    
    if (right->getType() != Value::typeFunction)
    {
        env->setRuntimeError(kscript::rterrorTypeError);
        return;
    }
        
    if (!left->isObject())
    {
        retval->setBoolean(false);
        return;
    }
    

    retval->setBoolean(left->getObject()->isKindOf(info->m_name.c_str()) ? true : false);
}


/*
static int hex2int(const wchar_t* hex)
{
    static const wchar_t* hexchars = L"0123456789ABCDEF";
    
    while (iswspace(*hex))
        hex++;
        
    bool neg = false;
    if (*hex == L'-')
    {
        neg = true;
        hex++;
    }
    
    if (*hex == L'0')
        hex++;
        
    if (*hex == L'x' || *hex == L'X')
        hex++;
    
    const wchar_t* end = hex;
    const wchar_t* p = hex;
    
    while (*p && NULL != wcschr(hexchars, towupper(*p)))
        p++;
    p--;
        
    int result = 0;
    int multiplier = 1;
    while (p >= end)
    {
        const wchar_t* pos = wcschr(hexchars,  towupper(*p));
        int a = pos-hexchars;
        result += (a*multiplier);
        multiplier *= 16;
        p--;
    }

    return neg ? -result : result;
}
*/


void jsparseInt(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count == 0)
    {
        retval->setNaN();
        return;
    }
    
    int base = 0;
    wchar_t* wc = env->m_eval_params[0]->getString();

    if (env->getParamCount() > 1)
    {
        base = env->getParam(1)->getInteger();
        if (base != 0 && (base < 2 || base > 36))
        {
            retval->setNaN();
            return;
        }
    }
    

    wchar_t* endp = NULL;
    long res = wcstol(wc, &endp, base);
    
    if (endp == wc)
    {
        retval->setNaN();
    }
     else
    {
        retval->setInteger(res);
    }
}

void jsparseFloat(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count == 0)
    {
        retval->setNaN();
        return;
    }

    if (env->m_eval_params[0]->isNumber())
    {
        env->m_eval_params[0]->toNumber(retval);
    }
     else if (env->m_eval_params[0]->isString())
    {
        wchar_t* ch = env->m_eval_params[0]->getString();
        while (isWhiteSpaceOrLS(*ch))
            ch++;
        
        if (wcsncmp(ch, L"+Infinity", 9) == 0 ||
            wcsncmp(ch, L"Infinity", 8) == 0)
        {
            retval->setPositiveInfinity();
            return;
        }
         else if (wcsncmp(ch, L"-Infinity", 9) == 0)
        {
            retval->setNegativeInfinity();
            return;
        }
         else if (wcsncmp(ch, L"NaN", 3) == 0)
        {
            retval->setNaN();
            return;
        }
         else
        {
            wchar_t* p = ch;
            while (*p)
            {
                if (!wcschr(L"0123456789.Ee-+ \t", *p))
                {
                    *p = 0;
                    break;
                }
                ++p;
            }
        }
        
        if (!*ch)
        {
            retval->setNaN();
            return;
        }
        
        const wchar_t* offending = NULL;
        parseNumericConstant(ch, retval, &offending, false);
    }
     else
    {
        retval->setNaN();
    }
}


void js_isNaN(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count == 0)
    {
        retval->setBoolean(true);
        return;
    }
    
    if (env->m_eval_params[0]->isNumber())
    {
        retval->setBoolean(env->m_eval_params[0]->isNaN());
    }
     else
    {
        Value v;
        env->m_eval_params[0]->toNumber(&v);
        retval->setBoolean(v.isNaN());
    }
}

void js_isFinite(ExprEnv* env, void* param, Value* retval)
{
    if (env->m_param_count == 0)
    {
        retval->setBoolean(false);
        return;
    }
    
    Value val;
    env->m_eval_params[0]->toNumber(&val);
    
    bool result = true;
    if (val.isNaN() || val.isAnyInfinity())
        result = false;
    
    retval->setBoolean(result);
}


void js_toString(ExprEnv* env, void* param, Value* retval)
{
    Value* vthis = env->getThis();

    if (!vthis)
    {
        retval->setNull();
        return;
    }

    vthis->toString(retval);
}


// Javascript operators


void oper_typeof(ExprEnv* env, void* param, Value* retval)
{
    int type = Value::typeUndefined;
    Value* v = env->m_params[0]->getLvalue(NULL);
    Value val;
    
    if (!v)
    {
        env->m_params[0]->eval(env, &val);
        v = &val;
        if (env->getRuntimeError() == rterrorReferenceError)
        {
            // if typeof was called on an undefined variable
            // mask the reference error
            env->setRuntimeError(0);
            val.setUndefined();
        }
    }
    
    type = v->getType();

    switch (type)
    {
        case Value::typeNull:        retval->setString(L"object");    break;
        case Value::typeUndefined:   retval->setString(L"undefined"); break;
        case Value::typeBoolean:     retval->setString(L"boolean");   break;
        case Value::typeInteger:     retval->setString(L"number");    break;
        case Value::typeDouble:      retval->setString(L"number");    break;
        case Value::typeString:      retval->setString(L"string");    break;
        case Value::typeBinary:      retval->setString(L"binary");    break;
        case Value::typeDateTime:    retval->setString(L"datetime");  break;
        case Value::typeFunction:    retval->setString(L"function");  break;
        case Value::typeRef:         retval->setString(L"ref");       break;
        case Value::typeObject:
        {
            ValueObject* obj = v->getObject();
            if (obj->isKindOf(Function::staticGetClassId()))
                retval->setString("function");
                 else
                retval->setString(L"object");
            break;
        }

        default:
            retval->setString(L"");
            break;
    }
}

void oper_void(ExprEnv* env, void* param, Value* retval)
{
    Value* v = env->m_params[0]->getLvalue(NULL);
    if (v)
    {
        retval->setUndefined();
    }
     else
    {
        Value val;
        env->m_params[0]->eval(env, &val);
    }
    
    retval->setUndefined();
}


void oper_delete(ExprEnv* env, void* param, Value* retval)
{
    ExprOperArrayInfo arrinfo;
    arrinfo.noeval = true;
    env->m_valreturn_arrinfo = &arrinfo;
    Value* obj = env->m_params[0]->getLvalue(env);
    env->m_valreturn_arrinfo = NULL;
    if (!obj || (ExprElement*)obj == env->m_params[0])
    {
        if (arrinfo.left.isNull())
        {
            retval->setBoolean(true);
            return;
        }
    }
    
    
    if (arrinfo.left.isRef())
        arrinfo.left.getRef()->eval(env, &arrinfo.left);


    if (arrinfo.left.isNull())
    {
        retval->setBoolean(false);
        
        if (obj->getDontDelete())
            return;
        
        ValueObject* global_obj = env->getParser()->getGlobalObject()->getObject();
        std::wstring name = global_obj->lookupMember(obj, false);
        retval->setBoolean(global_obj->deleteMember(name));
    }
     else
    {
        ValueObject* vobj = arrinfo.left.getObject();
        if (vobj)
            retval->setBoolean(vobj->deleteMember(arrinfo.right));
             else
            retval->setBoolean(true);
    }
}



void oper_comma(ExprEnv* env, void* param, Value* retval)
{
    Value v;
    env->m_params[0]->eval(env, &v);
    env->m_params[1]->eval(env, retval);
}



// some sql operators

void oper_like(ExprEnv* env, void* param, Value* retval)
{
    retval->setBoolean(false);
    
    if (env->m_param_count != 2)
        return;
        
    kscript::Value* p0 = env->getParam(0);
    kscript::Value* p1 = env->getParam(1);
    wchar_t* p0_str;
    wchar_t* p1_str;
    size_t p0_len;
    size_t p1_len;
       

    
    if (env->getParam(0)->isDateTime())
    {
        // date times are printed differently
        kscript::ExprDateTime dt;
        p0->getDateTime(&dt);
        
        int yy,mm,dd,h,m,s;
        
        
        kscript::julianToDate(dt.date, &yy, &mm, &dd);
        
        if (p0->getDataLen() == 8)
        {
            h = dt.time / 3600000;
            dt.time -= (h * 3600000);

            m = dt.time / 60000;
            dt.time -= (m * 60000);

            s = dt.time / 1000;
            dt.time -= (s * 1000);
            
            wchar_t buf[255];
            swprintf(buf, 255, L"%04d-%02d-%02d %02d:%02d:%02d", yy,mm,dd,h,m,s);
            p0->setString(buf);
        }
         else
        {
            wchar_t buf[255];
            swprintf(buf, 255, L"%04d-%02d-%02d", yy,mm,dd);
            p0->setString(buf);
        }
    }


    p0_str = p0->getString();
    p1_str = p1->getString();
    p0_len = p0->getDataLen();
    p1_len = p1->getDataLen();

    
    
    if (p1_len == 0)
    {
        retval->setBoolean(p0_len == 0 ? true : false);
        return;
    }
    

    bool left = (*p1_str == '%' ? true : false);
    bool right = (*(p1_str+p1_len-1) == '%' ? true : false);
    
    if (left && p1_len == 1)
    {
        // a single percent matches everything
        retval->setBoolean(true);
        return;
    }
    
    wchar_t* p = p1_str;
    if (left)
        p++;
    if (right)
        *(p1_str+p1_len-1) = 0;
    
    if (left && right)
    {
        // simple contains
        retval->setBoolean(wcsstr(p0_str, p) != NULL ? true : false);
        return;
    }
    
    if (right)
    {
        retval->setBoolean(wcsncmp(p0_str, p, p1_len-1) == 0 ? true : false);
        return;
    }
     else if (left)
    {
        if (p0_len < p1_len-1)
        {
            // left is shorter than right, return false
            retval->setBoolean(false);
            return;
        }
        
        retval->setBoolean(wcsncmp(p0_str+p0_len-(p1_len-1), p, p1_len-1) == 0 ? true : false);
        return;
    }
     else
    {
        // left is false and right is false (simple compare)
        retval->setBoolean(wcscmp(p0_str, p) == 0 ? true : false);
    }
}

void oper_notlike(ExprEnv* env, void* param, Value* retval)
{
    oper_like(env, param, retval);
    retval->setBoolean(!retval->getBoolean());
}


void oper_concat(ExprEnv* env, void* param, Value* retval)
{
    std::wstring res;
    size_t i;
    for (i = 0; i < env->m_param_count; ++i)
        res += env->m_eval_params[i]->getString();
    retval->setString(res);
}



};

