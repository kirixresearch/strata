/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2008-09-19
 *
 */


#ifndef __KSCRIPT_FUNCTIONS_H
#define __KSCRIPT_FUNCTIONS_H


namespace kscript
{


// built-in operators, functions and classes
void oper_array(ExprEnv* env, void* param, Value* retval);
void oper_bitwise_and(ExprEnv* env, void* param, Value* retval);
void oper_bitwise_xor(ExprEnv* env, void* param, Value* retval);
void oper_bitwise_or(ExprEnv* env, void* param, Value* retval);
void oper_bitwise_not(ExprEnv* env, void* param, Value* retval);
void oper_shift_left(ExprEnv* env, void* param, Value* retval);
void oper_shift_right(ExprEnv* env, void* param, Value* retval);
void oper_shift_right_zf(ExprEnv* env, void* param, Value* retval);
void oper_negate(ExprEnv* env, void* param, Value* retval);
void oper_unary_plus(ExprEnv* env, void* param, Value* retval);
void oper_not(ExprEnv* env, void* param, Value* retval);
void oper_and(ExprEnv* env, void* param, Value* retval);
void oper_or(ExprEnv* env, void* param, Value* retval);
void oper_increment(ExprEnv* env, void* param, Value* retval);
void oper_decrement(ExprEnv* env, void* param, Value* retval);
void oper_assign(ExprEnv* env, void* param, Value* retval);
void oper_not_equals(ExprEnv* env, void* param, Value* retval);
void oper_equals(ExprEnv* env, void* param, Value* retval);
void oper_less_than(ExprEnv* env, void* param, Value* retval);

void oper_greater_than(ExprEnv* env, void* param, Value* retval);
void oper_less_than_eq(ExprEnv* env, void* param, Value* retval);
void oper_greater_than_eq(ExprEnv* env, void* param, Value* retval);
void oper_mod(ExprEnv* env, void* param, Value* retval);
void oper_divide(ExprEnv* env, void* param, Value* retval);
void oper_multiply(ExprEnv* env, void* param, Value* retval);
void oper_add(ExprEnv* env, void* param, Value* retval);
void oper_subtract(ExprEnv* env, void* param, Value* retval);
void oper_pluseq(ExprEnv* env, void* param, Value* retval);
void oper_minuseq(ExprEnv* env, void* param, Value* retval);
void oper_multeq(ExprEnv* env, void* param, Value* retval);
void oper_diveq(ExprEnv* env, void* param, Value* retval);
void oper_modeq(ExprEnv* env, void* param, Value* retval);
void oper_bitandeq(ExprEnv* env, void* param, Value* retval);
void oper_bitoreq(ExprEnv* env, void* param, Value* retval);
void oper_bitxoreq(ExprEnv* env, void* param, Value* retval);
void oper_shleq(ExprEnv* env, void* param, Value* retval);
void oper_shreq(ExprEnv* env, void* param, Value* retval);
void oper_shrzfeq(ExprEnv* env, void* param, Value* retval);
void oper_in(ExprEnv* env, void* param, Value* retval);
void oper_instanceof(ExprEnv* env, void* param, Value* retval);
void oper_not_strict_equals(ExprEnv* env, void* param, Value* retval);
void oper_strict_equals(ExprEnv* env, void* param, Value* retval);
void oper_typeof(ExprEnv* env, void* param, Value* retval);
void oper_void(ExprEnv* env, void* param, Value* retval);
void oper_delete(ExprEnv* env, void* param, Value* retval);
void oper_comma(ExprEnv* env, void* param, Value* retval);
void oper_like(ExprEnv* env, void* param, Value* retval);
void oper_notlike(ExprEnv* env, void* param, Value* retval);
void oper_concat(ExprEnv* env, void* param, Value* retval);

// TODO: need to document; also, need to find where eval is
// and document it as well
void jsparseInt(ExprEnv* env, void* param, Value* retval);
void jsparseFloat(ExprEnv* env, void* param, Value* retval);
void js_isNaN(ExprEnv* env, void* param, Value* retval);
void js_isFinite(ExprEnv* env, void* param, Value* retval);
void js_toString(ExprEnv* env, void* param, Value* retval);


};



#endif

