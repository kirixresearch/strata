/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2008-10-10
 *
 */


#ifndef __KSCRIPT_ELEMENTS_H
#define __KSCRIPT_ELEMENTS_H


namespace kscript
{


class ExprEmptyStatement : public ExprElement
{
public:

    ExprEmptyStatement();
    ~ExprEmptyStatement();

    int eval(ExprEnv* env, Value* retval);
    int getType();
};


class ExprNew : public ExprElement
{
public:

    ExprNew();
    ~ExprNew();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprElement* m_expr;
};


class ExprThis : public ExprElement
{
public:

    ExprThis();
    int eval(ExprEnv* env, Value* retval);
    int getType();
    Value* getLvalue(ExprEnv* env);
};


class ExprSuper : public ExprElement
{
public:

    ExprSuper();
    ~ExprSuper();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    Value* getLvalue(ExprEnv* env);

public:

    ExprClassInfo* m_base_class_info;
    Value m_val;
};


class ExprVarLookup : public ExprElement
{
public:

    enum
    {
        modeVarId = 1,
        modeObjectMember = 2,
        modeRuntime = 3
    };

public:

    ExprVarLookup();
    ~ExprVarLookup();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    Value* getLvalue(ExprEnv* env);

public:

    int m_mode;
    int m_var_id;
    int m_retval_type;
    std::wstring m_symbol;
    
    ExprParser* m_parser;
    ExprParserEnv* m_scope;
    wchar_t* m_location;
};


class ExprVarDefine : public ExprElement
{
public:

    ExprVarDefine();
    ~ExprVarDefine();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprParserEnv* m_scope;
    int m_var_id;
    ExprElement* m_init;
};


class ExprVarAssign : public ExprElement
{
public:

    ExprVarAssign();
    ~ExprVarAssign();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    int m_var_id;
    ExprElement* m_init;
};


class ExprSimpleArrayLookup : public ExprElement
{
public:

    ExprSimpleArrayLookup();
    ~ExprSimpleArrayLookup();
    
    int eval(ExprEnv* env, Value* retval);
    int getType();
    Value* getLvalue(ExprEnv* env);
    
public:
    
    ExprElement* m_left;
    std::wstring m_right;
};




class ExprWith : public ExprElement
{
public:

    ExprWith();
    ~ExprWith();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprElement* m_with;
    ExprElement* m_sequence;
};


class ExprFunctionInit : public ExprElement
{
public:

    ExprFunctionInit(Function* func);
    ~ExprFunctionInit();
    
    int eval(ExprEnv* env, Value* retval);
    int getType();
    
private:

    Function* m_function;
};

class ExprArrayInit : public ExprElement
{
public:

    ExprArrayInit();
    ~ExprArrayInit();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:
    std::vector<ExprElement*> m_elements;
};



class ExprObjectLiteralItem
{
public:
    std::wstring m_name;
    ExprElement* m_varinit;
};


class ExprObjectLiteralInit : public ExprElement
{
public:

    ExprObjectLiteralInit();
    ~ExprObjectLiteralInit();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:
    std::vector<ExprObjectLiteralItem> m_elements;
};


class ExprFunctionCall : public ExprElement
{
public:

    ExprFunctionCall();
    ~ExprFunctionCall();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprElement* m_func_ptr;
    ExprElement** m_params;
    size_t m_param_count;
    int m_retval_type;
    std::wstring m_call_text;
};


class ExprFunctionReturn : public ExprElement
{
public:

    ExprFunctionReturn();
    ~ExprFunctionReturn();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprElement* m_expr;
};

class ExprBreak : public ExprElement
{
public:

    ExprBreak();
    ~ExprBreak();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    
public:

    int m_label_id;
};

class ExprContinue : public ExprElement
{
public:

    ExprContinue();
    ~ExprContinue();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    
public:

    int m_label_id;
};


class ExprInstruction : public ExprElement
{
public:

    ExprInstruction();
    virtual ~ExprInstruction();

    int eval(ExprEnv* env, Value* retval);
    int getType();

    Value* getLvalue(ExprEnv* env);

public:
    ExprElement** m_params;  // parameters
    Value** m_eval_params;   // evaluated parameters
    int m_param_count;       // parameter count
    ExprBindFunc m_func_ptr; // pointer to the built-in function or operator
    int m_retval_type;       // return value type
    bool m_prefix;           // for unary operators: true if prefix, false if postfix
    bool m_defer_calc;       // true if the parameter evaluation should not be performed before function invocation
    int m_exec_count;        // number of times instruction is being executed
};


class ExprIf : public ExprElement
{
public:
    ExprElement* m_condition;
    ExprElement* m_iftrue;
    ExprElement* m_iffalse;

    ExprIf();
    ~ExprIf();

    int eval(ExprEnv* env, Value* retval);
    int getType();
};

class ExprForWhile : public ExprElement
{
public:
    ExprForWhile();
    ~ExprForWhile();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:
    ExprElement* m_init;
    ExprElement* m_condition;
    ExprElement* m_loop_oper;
    ExprElement* m_sequence;
    bool m_condition_at_end;
    int m_label_id;
};

class ExprForIn : public ExprElement
{
public:
    ExprForIn();
    ~ExprForIn();

    int eval(ExprEnv* env, Value* retval);
    int getType();

public:
    ExprElement* m_init;
    ExprElement* m_element;
    ExprElement* m_arr;
    ExprElement* m_sequence;
    int m_label_id;
};



class ExprSwitchCase
{
public:
    ExprElement* case_element;
    size_t sequence_offset;
};

class ExprSwitch : public ExprElement
{
public:
    ExprSwitch();
    ~ExprSwitch();
    
    int eval(ExprEnv* env, Value* retval);
    int getType();

public:

    ExprElement* m_expr;
    ExprSequence* m_sequence;
    std::vector<ExprSwitchCase> m_cases;
    size_t m_default_count;
    int m_label_id;
};



class ExprSequence : public ExprElement
{
friend class ExprParser;
friend class ExprParserEnv;

public:

    ExprSequence();
    virtual ~ExprSequence();

    void addFunctionElement(ExprElement* inst);
    void addVarElement(ExprElement* inst);
    void prependElement(ExprElement* inst);
    
    void appendElement(ExprElement* inst);
    size_t getElementCount();
    
    int eval(ExprEnv* env, Value* retval);
    int getType();

protected:

    // the instructions vector and the instructions
    // array have the same content; during execution
    // a copy has to be made of the vector so that
    // adding new elements doesn't corrupt the array
    std::vector<ExprElement*> m_instructions;
    ExprElement** m_instructions_arr;
    size_t m_instructions_count;
    
    size_t m_func_count;
    size_t m_var_count;

    ExprParserEnv* m_penv;
    bool m_always_wants_return;
    bool m_create_env;
    
    int m_label_id;
};


class ExprTryCatch : public ExprElement
{
public:

    ExprTryCatch();
    virtual ~ExprTryCatch();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    
public:

    ExprElement* m_try_block;
    ExprElement* m_catch_block;
    ExprElement* m_finally_block;
    int m_catch_param;
};


class ExprThrow : public ExprElement
{
public:

    ExprThrow();
    virtual ~ExprThrow();

    int eval(ExprEnv* env, Value* retval);
    int getType();
    
public:

    ExprElement* m_expr;
};


class ExprArguments : public ExprElement
{
public:

    ExprArguments();
    virtual ~ExprArguments();

    int eval(ExprEnv* env, Value* retval);
    int getType();
};


};



#endif
