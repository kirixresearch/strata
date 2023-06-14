/*!
 *
 * Copyright (c) 2001-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  ECMA Script Library
 * Author:   Benjamin I. Williams
 * Created:  2001-10-08
 *
 */


#ifndef H_KSCRIPT_KSCRIPT_H
#define H_KSCRIPT_KSCRIPT_H


#include <vector>
#include <string>
#include <map>
#include <set>
#include <stack>
#include <cstdlib>


namespace kscript
{



enum ExprDialects
{
    optionLanguageECMAScript = 1 << 0,
    optionLanguageGeneric = 1 << 1,
    optionLanguageCustom = 1 << 2,
    optionNoAutoBindings = 1 << 3,
    optionEpsilonNumericCompare = 1 << 4,
    optionNoAutoSemicolon = 1 << 5,
    optionForceVarDeclare = 1 << 6,
    optionStrictClassScoping = 1 << 7
};



enum ExprResult
{
    evalSucceeded = 0,
    evalSucceededReturn = 1,
    evalSucceededBreak = 2,
    evalSucceededContinue = 3,
    evalFailed = -1,
    evalFailedCancelled = -2,
};


enum ExprCompileError
{
    errorNone = 0,
    errorMissingOpenBrace = 2,
    errorMissingCloseBrace = 3,
    errorMissingOpenBracket = 4,
    errorMissingCloseBracket = 5,
    errorMissingOpenParenthesis = 6,
    errorMissingCloseParenthesis = 7,
    errorMissingSemicolon = 8,
    errorMissingColon = 9,
    errorMissingOperator = 10,
    errorMissingQuote = 11,
    errorMissingCatchFinally = 12,
    errorMissingBaseClass = 13,
    errorInvalidIncludeFile = 14,
    errorMalformedStatement = 15,
    errorUndeclaredIdentifier = 16,
    errorVarRedefinition = 17,
    errorInvalidParameterTypes = 18,
    errorInvalidIdentifier = 19,
    errorUnterminatedString = 20,
    errorUnterminatedComment = 21,
    errorMalformedNumericConstant = 22,
    errorNotInClassScope = 23,
    errorUnknownEscapeSequence = 24,
    errorUnexpectedToken = 25,
    errorSyntaxError = 26,
    errorOutOfMemory = 998,
    errorUnknown = 999
};


enum ExprRuntimeError
{
    rterrorNone = 0,
    rterrorThrownException = 1,
    rterrorSyntaxError = 2,
    rterrorTypeError = 3,
    rterrorRangeError = 4,
    rterrorReferenceError = 5,
    rterrorNoEntryPoint = 6,
    rterrorMissingObject = 7,
    rterrorTermNotFunction = 8,
    rterrorObjectTypeMismatch = 9,
    rterrorExit = 900,
    rterrorUnknown = 999
};



// forward decalarations

class ValueObjectEvents;
class ValueObject;
class Value;
class ExprEnv;
class ExprInstruction;
class ExprOperator;
class Function;
class ExprVarBinding;
class ExprVarLookup;
class ExprParser;
class ExprParserEnv;
class ExprElement;
class ExprClassInfo;
class ExprSequence;
class ExprSwitch;


// int64 support

#ifdef _MSC_VER
typedef __int64 ks_int64_t;
typedef unsigned __int64 ks_uint64_t;
#else
typedef long long ks_int64_t;
typedef unsigned long long ks_uint64_t;
#endif


// parse hook info

struct ExprParseHookInfo
{
    enum
    {
        typeOperator = 1 << 0,
        typeFunction = 1 << 1,
        typeIdentifier = 1 << 2,
        typeInclude = 1 << 3,
        typeIncludePop = 1 << 4
    };


    // information about the text being parsed
    
    ExprParser* parser;      // ptr to the expression parser
    ExprParserEnv* penv;     // expression parser environment
    int element_type;        // type of expression
    void* hook_param;        // parameter passed to setParseHook
    std::wstring expr_text;  // full text of the expression


    // other information if element_type is equal to typeOperator

    ExprOperator* oper_info; // operator info
    std::wstring oper_text;  // operator text
    std::wstring oper_left;  // left side of operator (or unary operator text)
    std::wstring oper_right; // right side of the operator (blank if unary)
    bool oper_unary;         // true if the operator is unary
    bool oper_prefix;        // true if the unary operator prefixes the argument
    

    // result of the parse hook goes here
    ExprElement* res_element; // element result goes here
    std::wstring res_string;  // string result goes here (for typeInclude)
};


// function typedefs

typedef void (*ExprBindFunc)(ExprEnv* env, void* param, Value* retval);

typedef bool (*ExprParseHookFunc)(ExprParseHookInfo& info);

typedef int (*ExprDoubleCompareFunc)(double, double);


#ifdef _DEBUG
void debugBreak();
#else
inline void debugBreak() { }
#endif


// class declarations

class ExprElement
{
public:
    ExprElement() { }
    virtual ~ExprElement() { }

    virtual int eval(ExprEnv* env, Value* retval) = 0;
    virtual int getType() = 0;
    virtual Value* getLvalue(ExprEnv* env) { return NULL; }
};


class ExprDateTime
{
public:
    int date;      // julian day
    int time;      // milliseconds since midnight
};



class ShadowObject
{
public:
    virtual bool getMember(const std::wstring& name, Value* retval) = 0;
};


class RefObject
{
public:
    
    int m_base_ref_count;

    RefObject() { m_base_ref_count = 0; }
    virtual ~RefObject() { }

    void baseRef() { m_base_ref_count++; }
    void baseUnref() { if (--m_base_ref_count == 0) delete this; }
};



class ValueMember
{
public:
    std::wstring name;
    Value* value;
};

class ValueObject : public RefObject
{
friend class Value;
friend class Array;

public:

    ValueObject();
    ~ValueObject();

    virtual Value* createMember(const std::wstring& name, unsigned int props);
    virtual void setMember(const std::wstring& name, Value* value);
    virtual void getMember(const std::wstring& name, Value* retval);
    virtual Value* getMember(const std::wstring& name);
    virtual bool getMemberExists(const std::wstring& name);
    virtual bool deleteMember(const std::wstring& name);
    virtual void deleteAllMembers();
    virtual Value* lookupMember(const std::wstring& name, bool follow_prototype = true);
    virtual std::wstring lookupMember(Value* val, bool follow_prototype = true);

    virtual void setMemberI(int idx, Value* value);
    virtual void getMemberI(int idx, Value* retval);
    virtual Value* getMemberI(int idx);
    virtual bool deleteMemberI(int idx);
    virtual void appendMember(Value* value);
    int getHighestIndex() { return m_highest_index; }    
    
    virtual std::wstring getRawMemberName(size_t idx);
    virtual size_t getRawMemberCount();
    virtual Value* getRawMemberByIdx(size_t idx);
    
    virtual void toString(Value* retval);
    virtual void toNumber(Value* retval);
    virtual void toBoolean(Value* retval);
    virtual void toPrimitive(Value* retval, int preferred_type);

    virtual bool isKindOf(const wchar_t* class_name) const;
    virtual bool isKindOf(int class_id) const;
    virtual void setClassName(const std::wstring& class_name);
    virtual const std::wstring& getClassName() const;
    virtual int getClassId() const { return 0; }

    virtual Value* getPrototype();
    virtual void setPrototype(Value* val);
    virtual bool hasPrototype();
    
    void setShadowObject(ShadowObject* shadow) { m_shadow = shadow; }
    
    void registerEventListener(ValueObjectEvents* evt);
    void unregisterEventListener(ValueObjectEvents* evt);

    ExprParser* getParser() { return m_parser; }
    void setParser(ExprParser* e) { m_parser = e; }

    static void staticConstructor(ExprEnv* env, void* param, Value* retval);
    static bool customConstructor(int class_id, ExprEnv* env, void* param, Value* retval);
    static const std::wstring& staticGetClassName() { static std::wstring c; return c; }
    static bool staticIsKindOf(const wchar_t* klass_name) { return false; }
    static bool staticIsKindOf(int klass_name) { return false; }
    static void addPrototype(ValueObject* obj) { }
    
protected:


    void setHighestIndex(int val) { m_highest_index = val; }
    
    const ValueMember& getRawValueMemberByIdx(size_t idx) { return m_members[idx]; }
    virtual const std::wstring& getDefaultClassName() const;
    
    Value* getValueReturn();
    
protected:

    std::wstring m_class_name;
    
private:

    std::vector<ValueMember> m_members;
    std::map<std::wstring, Value*> m_members_map;
    
    std::vector<ValueMember> m_nonenum_members;
    std::map<std::wstring, Value*> m_nonenum_members_map;
    
    std::vector<ValueObjectEvents*> m_valobj_events;
    
    kscript::ExprParser* m_parser;
    Value* m_value_return;          // used for member return values
    Value* m_prototype;
    ShadowObject* m_shadow;

    int m_highest_index; // highest used numeric index + 1
    bool m_in_destructor; // true if code is in destructor
};


class ValueObjectEvents
{
public:
    virtual ~ValueObjectEvents() { }
    virtual void onDestroy(ValueObject*) { }
};


class Value : public ExprElement
{
friend class ValueObject;
friend class ExprClassInfo;
friend class ExprInstruction;
friend class ExprFunctionCall;
friend class ExprEnv;
friend class ExprParser;

public:

    enum
    {
        typeNull = 0,
        typeUndefined = 1,
        typeObject = 2,
        typeBoolean = 3,
        typeInteger = 4,
        typeDouble = 5,
        typeString = 6,
        typeBinary = 8,
        typeDateTime = 9,
        typeFunction = 10,
        typeRef = 11,
        typeExprElement = 12
    };

    enum
    {
        methodNormal = 0,
        methodGetVal = 1
    };

    enum
    {
        attrNone = 0x00,
        attrDontEnum = 0x01,
        attrDontDelete = 0x02,
        attrReadOnly = 0x04
    };

public:

    Value();
    Value(int value);
    Value(double value);
    Value(const wchar_t* str, int len = -1);
    Value(const std::wstring& str);
    
    virtual ~Value();
    
    void clear();
    void reset();
    void releaseObject();
    void releaseScope();
    
    void setAttributes(unsigned char attr) { m_attr = attr; }
    unsigned char getAttributes() const { return m_attr; }
    
    bool getReadOnly() { return (m_attr & attrReadOnly) ? true : false; }
    bool getDontEnum() { return (m_attr & attrDontEnum) ? true : false; }
    bool getDontDelete() { return (m_attr & attrDontDelete) ? true : false; }
    
    bool isNaN();
    bool isAnyInfinity();
    bool isNegativeInfinity();
    bool isPositiveInfinity();
    bool isAnyZero();
    bool isNegativeZero();
    bool isPositiveZero();
    int getSign();
    
    int getType();
    bool isUndefined() { return (m_type == Value::typeUndefined ? true : false); }
    bool isNull() { return (m_type == Value::typeNull || m_type == Value::typeUndefined ? true : false); }
    bool isObject() { return (m_type == Value::typeObject || m_type == Value::typeFunction ? true : false); }
    bool isString() { return (m_type == typeString ? true : false); }
    bool isBoolean() { return (m_type == typeBoolean) ? true : false; }
    bool isNumber() { return ((m_type == typeDouble || m_type == typeInteger) ? true : false); }
    bool isDouble() { return (m_type == typeDouble ? true : false); }
    bool isInteger() { return (m_type == typeInteger ? true : false); }
    bool isDateTime() { return ((m_type == typeDateTime) ? true : false); }
    bool isBinary() { return (m_type == typeBinary ? true : false); }
    bool isFunction() { return ((m_type == typeFunction) ? true : false); }
    bool isRef() { return (m_type == typeRef ? true : false); }
    bool isExprElement() { return (m_type == typeExprElement ? true : false); }
    
    // hasObject() is slightly different than isObject(), in 
    // that it checks for the existence of an m_obj object,
    // apart from the actual value type.  isObject() only returns
    // true of typeof(val)=="object"
    bool hasObject() { return (m_obj ? true : false); }
    
    Value& setValue(Value* val) { val->eval(NULL, this); return *this; }
    Value& setValue(const Value& val) { ((Value&)val).eval(NULL, this); return *this; }
    
    void setType(int new_type) { m_type = new_type; }
    void setNull();
    void setUndefined() { reset(); }
    void setNaN();
    void setPositiveInfinity();
    void setNegativeInfinity();
    void setObject();
    void setObject(ValueObject* obj);
    void setArray(ExprEnv* env);
    void setString(const std::string& str);
    void setString(const std::wstring& str) { setString(str.c_str(), (int)str.length()); }
    void setString(const Value* src_string) { setString(src_string->m_strptr, src_string->m_datalen); }
    void setString(const wchar_t* str, int len = -1);
    void appendString(const wchar_t* str, int len = -1);
    void setBinary(unsigned char* data, int len, int copy_len = -1);
    void setBoolean(bool b);
    void setDouble(double num);
    void setInteger(int num);
    void setRef(Value* v);
    void setFunction(Function* func_info, ExprEnv* scope = NULL);
    void setFunction(ExprBindFunc func_ptr, void* binding_param = NULL);
    void setExprElement(ExprElement* expr);
    void setDateTime(int year,
                     int month,
                     int day,
                     int hour=0,
                     int min=0,
                     int sec=0,
                     int sec1000=0);

    void setDateTime(unsigned int date,
                     unsigned int time);

    void setGetVal(int var_type, ExprBindFunc func, void* param)
    {
        m_type = var_type;
        m_method = methodGetVal;
        m_getvalptr = func;
        m_getvalparam = param;
    }
    
    wchar_t* getString();
    unsigned char* getBinary() { return m_binptr; }
    
    int getDataLen() { return m_datalen; }
    void setDataLen(int new_value) { m_datalen = new_value; }
    
    void setScale(unsigned char new_value) { m_scale = new_value; }
    unsigned char getScale() { return m_scale; }
    
    ExprDateTime getDateTime();
    void getDateTime(ExprDateTime* dest);
    bool getBoolean();
    double getDouble();
    int getInteger();
    Value* getRef() { if (isRef()) return m_refptr; else return NULL; }
    ValueObject* getObject() { if (!m_obj) checkObjectExists(); return m_obj; }
    Function* getFunction() { return m_funcptrval; }
    ExprEnv* getFunctionScope() { return m_scope; }

    int eval(ExprEnv* env, Value* retval);
    Value* getLvalue(ExprEnv* env);

    Value* getPrototype() { return getObject()->getPrototype(); }
    void setPrototype(Value* val) { getObject()->setPrototype(val); }

public:

    void checkObjectExists();
    void setStringLen(unsigned int len);
    void allocString(unsigned int len);
    void allocMem(unsigned int len);
    
    Value* lookupMember(const std::wstring& name, bool follow_prototype = true);
    Value* getMember(const std::wstring& name);
    Value* getMemberI(int idx);
    bool getMemberExists(const std::wstring& name);
    void appendMember(Value* value);
    void setMember(const std::wstring& name, Value* value);
    void setMemberI(int idx, Value* value);
    Value* createMember(const std::wstring& name,
                        unsigned int props = Value::attrNone);
                        
    void toString(Value* retval);
    void toNumber(Value* retval);
    void toBoolean(Value* retval);
    void toPrimitive(Value* retval, int preferred_type = Value::typeUndefined);

private:

    void copyFrom(kscript::Value* src_val);

private:

    union
    {
        int m_intval;
        double m_dblval;
        bool m_boolval;
        ExprDateTime m_datetimeval;

        unsigned char* m_binptr;
        wchar_t* m_strptr;
        void* m_genericptr;
        Value* m_refptr;
        ExprElement* m_exprptr;
        
        struct
        {
            Function* m_funcptrval;
            ExprEnv* m_scope;
        };
        
        struct
        {
            ExprBindFunc m_getvalptr;
            void* m_getvalparam;
        };
    };

    ValueObject* m_obj;       // bound object
    Value* m_value_return;    // variable where method returns are placed
    unsigned int m_datalen;   // data length of a string
    unsigned int m_alloc_len; // amount of data allocated
    unsigned char m_type;     // type of Value (see type enum)
    unsigned char m_method;   // method of retrieving values (see method enum)
    unsigned char m_attr;     // value attributes
    unsigned char m_scale;    // scale of numeric value
};









class ExprClassInfo
{
public:

    ExprClassInfo(ExprParser* parser);
    ~ExprClassInfo();
    
    void addMember(const std::wstring& name, ExprElement* expr);
    void addFunction(const std::wstring& name, ExprBindFunc func, void* param);
    
    void addPrototypeMember(const std::wstring& name, Value* val);
    void addPrototypeFunction(const std::wstring& name, ExprBindFunc func, void* param);
    
    Value* addStaticMember(const std::wstring& name, Value* val);
    void addStaticFunction(const std::wstring& name, ExprBindFunc func, void* param);
    
    bool getMemberExistsInHierarchy(const std::wstring& name);
    bool getMemberExists(const std::wstring& name);
    Value* getPrototypeMember(const std::wstring& name);
    
    void setupObject(ValueObject* obj);
    ExprClassInfo* getBaseClass();
    
    Value* getClassObject() { return m_class_obj; }
    
public:
    std::wstring m_name;                            // name of the class
    std::wstring m_base_name;                       // name of the base class
    Value m_prototype;                              // prototype object for this class
    std::map<std::wstring, ExprElement*> m_members; // class members
    ExprClassInfo* m_base;                          // base class info
    ExprParser* m_parser;                           // ptr to owning parser
    ExprBindFunc m_objinit_func;                    // object initialization function
    Value* m_class_obj;                             // reference to the class object
    Function* m_cstdef;                             // constructor function
};


class ExprParserToken
{
public:
    std::wstring name;        // name of the token
    ExprVarBinding* binding;  // the binding where this token originated (optional)
    int id;                   // token id
    bool case_sense;          // whether the name should be treated as case-sensitive
};

class ExprParserEnv
{
public:

    ExprParserEnv(ExprParserEnv* parent);
    ~ExprParserEnv();

    void reserve(size_t amount);

    int declareToken(const std::wstring& token,
                     bool case_sense = true,
                     ExprVarBinding* binding = NULL);

    ExprParserToken* lookupToken(const std::wstring& token);

    bool getTokenExist(const std::wstring& token);
    bool getTokenExistInScope(const std::wstring& token);
    ExprParserToken* getToken(const std::wstring& token);
    int getTokenId(const std::wstring& token) { return getToken(token)->id; }

    ExprParserEnv* getHighestContext();
    ExprParserEnv* getVarContext();
    ExprParserEnv* getEvalContext();
    ExprSequence* getSequence();
    
    ExprClassInfo* getClassInfo();
    void setClass(const std::wstring& class_name);
    std::wstring getClass();
    bool getSuperCalled();

public:
    
    void addChild(ExprParserEnv* child);
    void removeChild(ExprParserEnv* child);
    ExprParserToken* _lookupToken(const std::wstring& token);

public:
    std::map<std::wstring, ExprParserToken*> m_tokens;
    std::map<std::wstring, ExprParserToken*> m_itokens;
    std::vector<ExprParserEnv*> m_children;
    ExprParserEnv* m_parent;
    ExprParser* m_parser;
    ExprSequence* m_seq;
    Function* m_func;
    
    std::wstring m_class;
    bool m_super_called;
    bool m_eval;
};



class ExprOperator
{
public:

    enum
    {
        typeNormal = 0,
        typeUnary = 1,
        typeParenthesized = 2
    };

    enum
    {
        directionRightToLeft = 0,
        directionLeftToRight = 1
    };

    std::wstring m_name;
    std::wstring m_name_close; // used by parenthesized operators only
    std::wstring m_formats;
    ExprBindFunc m_func_ptr;

    int m_priority;         // higher priority operators bind closer than lower priority operators
    int m_type;             // true if operator is unary
    bool m_defer_calc;      // true if calculations should not be performed prior to invoking operator's function
    int m_direction;        // right-to-left or left-to-right;
    bool m_alpha;           // true if the operator is uses only alphabet characters "and", "or", etc
    bool m_case_sense;         // true if variable name binding is case-sensitive
};

class ExprVarBinding
{
public:

    std::wstring m_name;       // variable name
    std::wstring m_formats;    // (only used if this is a bound function)
    int m_var_type;            // variable type
    int m_var_id;              // variable id (once the variable is bound)
    bool m_case_sense;         // true if variable name binding is case-sensitive
    Value* m_start_value;      // initial value of the variable

    std::wstring m_base_name;  // base class (class bindings only)
    ExprBindFunc m_class_init; // class initialization binding (classes only)
        
    ExprVarBinding()
    {
        m_name = L"";
        m_base_name = L"";
        m_formats = L"";
        m_var_type = 0;
        m_var_id = -1;
        m_case_sense = false;
        m_class_init = NULL;
        m_start_value = NULL;
    }

    ExprVarBinding(const ExprVarBinding& c)
    {
        m_name = c.m_name;
        m_base_name = c.m_base_name;
        m_formats = c.m_formats;
        m_var_type = c.m_var_type;
        m_var_id = c.m_var_id;
        m_case_sense = c.m_case_sense;
        m_class_init = c.m_class_init;
        m_start_value = c.m_start_value;
    }
};



class ExprErrorInfo
{
    friend class ExprParser;

public:

    int getCode() { return m_code; }
    int getLine() { return m_line; }
    size_t getOffset() { return m_offset; }
    const std::wstring& getText() { return m_text; }
    const std::wstring& getFileName() { return m_filename; }

private:

    int m_code;
    int m_line;
    size_t m_offset;
    std::wstring m_text;
    std::wstring m_filename;
};


class ExprParserSource
{
public:
    
    wchar_t* m_src;
    std::wstring m_file;
};


class ExprDepthCounter
{
public:

    ExprDepthCounter(int* _c) { c = _c; (*c)++; }
    ~ExprDepthCounter() { (*c)--; }
    
private:

    int* c;
};


class ExprParser
{
    friend class ExprParserEnv;
    friend class ExprEnv;
    friend class ExprInstruction;
    friend class ExprTryCatch;
    friend class ExprClassInfo;
    friend class ExprVarLookup;
    friend class ExprFunctionCall;
    friend class Function;
    friend class GlobalObject;
    friend class Json;
    
public:

    ExprParser(unsigned int flags);
    virtual ~ExprParser();

    void setParseHook(int parse_hook_mask,
                      ExprParseHookFunc func,
                      void* param);
                      
    void setExtraLong(uintptr_t val);
    uintptr_t getExtraLong();
    
    ExprOperator& addOperator(const std::wstring& oper_name,
                     bool case_sense,
                     ExprBindFunc func_ptr,
                     int priority,
                     int type,
                     int direction,
                     bool defer_calc,
                     const std::wstring& formats);

    ExprOperator& addOperator(const std::wstring& oper_open,
                     const std::wstring& oper_close,
                     bool case_sense,
                     ExprBindFunc func_ptr,
                     int priority,
                     int type,
                     int direction,
                     bool defer_calc,
                     const std::wstring& formats);
                     
    ExprClassInfo* addClass(const std::wstring& class_name,
                            const std::wstring& baseclass_name = L"",
                            kscript::ShadowObject* prototype_shadow = NULL,
                            ExprBindFunc initobj_func = NULL);

    // this function is virtual to prevent a bug in VC6 which can
    // cause cross-DLL referencing of the internal STL map;  this
    // compiler bug was subsequently fixed in VC7 and doesn't affect
    // other compilers such as GCC at all
    virtual Value* addFunction(const std::wstring& func_name,
                     bool case_sense,
                     ExprBindFunc func_ptr,
                     bool defer_calc,
                     const std::wstring& formats,
                     void* param = NULL);

    Value* addVarBinding(const std::wstring& var_name,
                     bool case_sense,
                     int var_type,
                     void* ptr,
                     void* param);

    Value* addDoubleValue(const std::wstring& var_name,
                        double value);
                         
    Value* addIntegerValue(const std::wstring& var_name,
                         int value);

    Value* addStringValue(const std::wstring& var_name,
                        const std::wstring& value);

    Value* addObjectValue(const std::wstring& var_name,
                        ValueObject* obj);

    Value* addNullValue(const std::wstring& var_name);

    Value* addValue(const std::wstring& var_name,
                  Value& val);

    int invoke(ExprEnv* env,
                Value* func,
                Value* vthis,
                Value* retval,
                Value** vparams = NULL,
                size_t param_count = 0);

    bool createObject(const std::wstring& class_name,
                      kscript::Value* obj);

    Value* getBindVariable(const std::wstring& bind_name);
    void setMaximumDepth(int max_depth);
    bool isCancelled() const { return m_cancelled; }
    
    void setFlags(unsigned int flags) { m_flags = flags; }
    unsigned int getFlags() const { return m_flags; }

    ExprErrorInfo getErrorInfo();

    bool parse(const std::wstring& expr);
    bool eval(Value* retval);
    void cancel();
    int getType();

public:

    ExprEnv* createEnv();
    void returnEnv(ExprEnv* env);

    ExprClassInfo* getClassInfo(const std::wstring& class_name);
    ExprClassInfo* getClassInfo(Value* value);
    bool getClassExist(const std::wstring& class_name);
    
    Value* getGlobalObject() { return &m_global_object; }
    ExprEnv* getGlobalScope() { return m_global_scope; }
    
    void bindObject(ValueObject* obj);
    
    static int allocateClassId();
    
    Function* createFunction(Value* prototype = NULL);

    ExprElement* createVariableLookup(ExprParserEnv* penv, const std::wstring& symbol);

    void setRuntimeError(int rterror_code,
                         Value* exception_obj,
                         const std::wstring& text);


protected:

    // these can be overridden by derived classes
    // for custom parsing behavior
    virtual kscript::ExprElement* onParseElement(
                     kscript::ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc = NULL) { return NULL; }
    
protected:

    static void jsEvalStatic(ExprEnv* env, void* param, Value* retval);
    void jsEval(ExprEnv* env, void* param, Value* retval);
    
protected:

    int getSymbolId(const std::wstring& symbol);
    const std::wstring& getSymbolName(int id);

    wchar_t* getNextOper(wchar_t* expr,
                         wchar_t* expr_end,
                         ExprOperator** entry);

    ExprElement* parseVar(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc,
                     ExprSequence* seq = NULL);

    ExprElement* parseFunction(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc,
                     ExprSequence* seq = NULL,
                     bool as_expression = false);
    
    ExprElement* parseStringLiteral(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc,
                     size_t prealloc_len = 0);
                                       
    ExprElement* parseRegexLiteral(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc);
                     
    ExprElement* parseObjectLiteral(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc);
                      
    ExprElement* parseArrayLiteral(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc);

    ExprElement* parseCase(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc,
                     ExprSwitch* switch_element,
                     size_t seq_offset);
                      
    ExprElement* parseDefault(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc,
                     ExprSwitch* switch_element,
                     size_t sequence_offset);
                      
    ExprElement* parseClass(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc);
                  

    bool parseParams(ExprParserEnv* penv,
                     wchar_t* params,
                     std::vector<ExprElement*>& res,
                     wchar_t** endloc = NULL);

    ExprElement* parseElement(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc = NULL);

    ExprElement* parseStatementElement(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc = NULL);
                     
    ExprElement* parseStatement(ExprParserEnv* penv,
                     wchar_t* expr,
                     wchar_t** endloc);

    ExprSequence* parseSequence(ExprParserEnv* penv,
                     wchar_t* expr,
                     ExprSwitch* opt_switch = NULL,
                     bool opt_eval = false);

    void calcErrorInfo();
    
    void addUnresolved(ExprVarLookup* v);
    void removeUnresolved(ExprVarLookup* v);
    bool resolveUnresolved();
    
protected:

    // parsing helpers
    
    wchar_t* zl_strchr(
                 wchar_t* str,
                 wchar_t ch,
                 const wchar_t* open_parens = L"(",
                 const wchar_t* close_parens = L")");
    
    wchar_t* zl_stristr(
                 wchar_t* str,
                 wchar_t* end,
                 const std::wstring& search_str,
                 bool single_word,
                 bool case_sense,
                 bool forward = true);

    bool zl_getParam(
                 wchar_t** _str,
                 wchar_t** result_start,
                 wchar_t** result_end,
                 const wchar_t* open_parens,
                 const wchar_t* close_parens);

    wchar_t* zl_findCloseBrace(wchar_t* str);

    wchar_t* zl_getNextToken(
                 wchar_t* str,
                 wchar_t** end);

    bool removeComments(wchar_t* expr);

protected:

    std::vector<ExprOperator> m_opers;               // operators
    std::vector<ExprVarBinding> m_bindings;          // variable bindings
    std::map<std::wstring,ExprClassInfo*> m_classes; // class information
    std::set<ExprVarLookup*> m_unresolved;           // helps implement forwards
    std::vector<ExprParserSource> m_sources;         // ptrs to source code

    std::stack<ExprEnv*> m_unused_envs;   // environment stack
    std::vector<ExprEnv*> m_envs;         // list of ExprEnv objects in use

    std::vector<ExprClassInfo*> m_class_lookup;  // lookup class info by class id

    ExprParseHookFunc m_parse_hook;  // parse hook
    void* m_parse_hook_param;        // parameter for parse hook function
    int m_parse_hook_mask;           // mask for desired parse hook expression types
    
    ExprSequence* m_entrypt;
    ExprEnv* m_global_scope;
    ExprParserEnv* m_root_penv;
    Value m_global_object;
    Value m_object_prototype;        // prototype of Object
    Value m_function_prototype;      // prototype of Function
    
    unsigned int m_flags;
    int m_retval_type;
    int m_current_depth;
    int m_max_depth;
    int m_language;
    bool m_bindings_defined;
    bool m_cancelled;
    uintptr_t m_extra_long;
    
    std::map<std::wstring, int> m_dictionary_m;
    std::vector<std::wstring> m_dictionary_v;
    int m_symbol_counter;
    
    Value m_exception_obj;
    int m_error_code;
    int m_error_line;
    int m_error_offset;
    const wchar_t* m_error_loc;
    std::wstring m_error_file;
    std::wstring m_error_text;
    
    int m_label_id; // label id for break/continue

public:

    ExprDoubleCompareFunc dblcompare; // function for comparing doubles
};




struct ExprOperArrayInfo
{
    ExprOperArrayInfo() { noeval = false; }
    kscript::Value left;
    std::wstring right;
    bool noeval;
};


class ExprEnv
{
public:

    ExprEnv();
    ~ExprEnv();


    int getRefCount() { return m_ref_count; }
    void ref() { m_ref_count++; }

    void unref()
    {
        #ifdef _DEBUG
        if (m_ref_count <= 0)
            debugBreak();
        #endif

        if (--m_ref_count == 0)
        {
            reset();
            if (m_parser)
                m_parser->returnEnv(this);
        }
    }

    inline ExprParser* getParser() const { return m_parser; }
    
    bool isCancelled()
    {
        if (!m_parser)
            return false;
        return m_parser->isCancelled();
    }

    size_t getParamCount() { return m_param_count; }
    int getParamType(size_t param);
    Value* getParam(size_t param);
    void* getBindingParam();
    
public:

    void reset();
    void resetCircular();
    
    void setParent(ExprEnv* p)
    {
        m_parent = p;
        if (m_parent)
            m_parent->ref();
    }
    
    
    int getRuntimeError() const
    {
        return m_parser->m_error_code;
    }

    void setRuntimeError(int rterror_code,
                         Value* exception_obj = NULL,
                         const std::wstring& text = L"")
    {
        m_parser->setRuntimeError(rterror_code, exception_obj, text);
    }
    
    int getBreakContinueLabel() { return m_parser->m_label_id; }
    void setBreakContinueLabel(int l) { m_parser->m_label_id = l; }
                         
    Value* getThis() { return m_val_this; }
    void setThis(Value* val_this) { m_val_this = val_this; }

    void reserveParams(size_t param_count);
    void reserveVars(size_t size);
    Value* declareVar(int var_id);
    Value* getValue(int var_id);

    ExprParserEnv* getParserContext();

    bool createObject(const std::wstring& class_name, Value* obj);

public:

    int m_ref_count;
    
    std::vector<Value*> m_vars;
    int m_min_var;              // minimum index used in m_var; -1 if unused
    int m_max_var;              // maximum index used in m_var; -1 if unused
    ExprEnv* m_parent;
    ExprParser* m_parser;
    ExprParserEnv* m_parser_env;

    Value* m_valreturn_this;    // array function places it's 'this' here
    ExprOperArrayInfo* m_valreturn_arrinfo;
    Value* m_val_with;          // value on which to perform 'with' lookups
    Value* m_val_this;          // the 'this' value that will be used when calling an object
    Value* m_newobj;
    size_t m_switchcase_offset; // if > 0, this is the offset of the case which should be run

    Value** m_eval_params;      // array containing the evaluated results on m_params
    ExprElement** m_params;     // array of parameters
    size_t m_param_count;       // number of parameters
    size_t m_param_array_size;  // size of the parameters arrays
    Value* m_arguments;         // value for 'arguments' object (only created if necessary)
    Function* m_callee;         // callee pointer (hot, not ref'ed)
    
    void* m_binding_param;
    bool m_prefix;              // true if call was a prefix operator, false otherwise
    
    Value* m_null_value;         // this is used to return a null value (it's always null)
};


// helper functions for the parser

void str2date(ExprDateTime* dt,
              const wchar_t* str,
              const wchar_t* format = L"");



// ugly macros that make binding to a C++ object easy


// base class for all BoundMemberInfo objects -- this aids us
// in destroying these objects when we are done with them
class BoundMemberInfoBase
{
public:
    virtual ~BoundMemberInfoBase() { }
};


#define _BEGIN_KSCRIPT_CLASS(class_name, cpp_class, cpp_baseclass) \
public:\
typedef void (cpp_class::*BoundClassMethod)(kscript::ExprEnv*, kscript::Value*); \
static const std::wstring& staticGetClassName() { static std::wstring c(L##class_name); return c; } \
static int staticGetClassId() { \
    static int id = 0; \
    if (!id) id = kscript::ExprParser::allocateClassId(); \
    return id; \
} \
static bool staticIsKindOf(const wchar_t* klass_name) { \
    if (staticGetClassName() == klass_name) \
        return true; \
    return cpp_baseclass::staticIsKindOf(klass_name); \
} \
static bool staticIsKindOf(int klass_id) { \
    if (staticGetClassId() == klass_id) \
        return true; \
    return cpp_baseclass::staticIsKindOf(klass_id); \
} \
static void addPrototype(kscript::ValueObject* obj) { \
    obj->getPrototype()->getObject()->setShadowObject(cpp_class::getShadowObject()); \
    cpp_baseclass::addPrototype(obj->getPrototype()->getObject()); \
} \
static cpp_class* createObject() { \
    cpp_class* obj = new cpp_class; \
    cpp_class::addPrototype(static_cast<kscript::ValueObject*>(obj)); \
    return obj; \
} \
static cpp_class* createObject(kscript::ExprEnv* e) { \
    return createObject(e->getParser()); \
} \
static cpp_class* createObject(kscript::ExprParser* e) { \
    cpp_class* obj = new cpp_class; \
    if (e) e->bindObject(obj); \
    return obj; \
} \
const std::wstring& getDefaultClassName() const { return cpp_class::staticGetClassName(); } \
bool isKindOf(const wchar_t* klass_name) const { \
    return staticIsKindOf(klass_name); \
} \
bool isKindOf(int klass_id) const { \
    return staticIsKindOf(klass_id); \
} \
int getClassId() const { \
    return cpp_class::staticGetClassId(); \
} \
union BoundClassMethodHolder { \
    BoundClassMethod method; \
    kscript::ks_uint64_t p; \
}; \
class BoundMemberInfo : public kscript::BoundMemberInfoBase { \
public: \
    BoundMemberInfo() { \
        class_method.p = 0; \
        func_ptr = NULL; \
        is_static = false; \
        method_name = NULL; \
        param = NULL; \
        class_id = 0; \
        prototype = false; \
    } \
    BoundMemberInfo(const wchar_t* _name, kscript::ExprBindFunc _funcptr, void* _param, int _class_id, bool _prototype) { \
        class_method.p = 0; \
        func_ptr = (void*)_funcptr; \
        is_static = true; \
        method_name = _name; \
        param = _param; \
        class_id = _class_id; \
        prototype = _prototype; \
    } \
    BoundMemberInfo(const wchar_t* _name, BoundClassMethod _funcptr, void* _param, int _class_id, bool _prototype) { \
        class_method.method = _funcptr; \
        func_ptr = NULL; \
        is_static = false; \
        method_name = _name; \
        param = _param; \
        class_id = _class_id; \
        prototype = _prototype; \
    } \
    bool is_static; \
    const wchar_t* method_name; \
    void* func_ptr; \
    void* param; \
    BoundClassMethodHolder class_method; \
    kscript::Value value; \
    int class_id; \
    bool prototype; \
}; \
static void object_factory_func(kscript::ExprEnv* env, \
                                void* param, \
                                kscript::Value* retval) { \
    kscript::Value* thisobj = env->getThis(); \
    if (!thisobj) { \
        staticConstructor(env, param, retval); \
        return; \
    } \
    if (customConstructor(cpp_class::staticGetClassId(), env, param, retval)) \
        return; \
    cpp_class* obj = new cpp_class; \
    thisobj->setObject(obj); \
} \
static void bind_func(kscript::ExprEnv* env, \
                      void* param, \
                      kscript::Value* retval) { \
    if (!env) return; \
    kscript::Value* thisobj = env->getThis(); \
    if (!thisobj) { \
        env->setRuntimeError(kscript::rterrorMissingObject); \
        return; \
    } \
    kscript::ValueObject* vobj = thisobj->getObject(); \
    BoundMemberInfo* method_info = (BoundMemberInfo*)param; \
    if (!vobj->isKindOf(method_info->class_id)) { \
        env->setRuntimeError(kscript::rterrorObjectTypeMismatch); \
        return; \
    } \
    cpp_class* obj = (cpp_class*)vobj; \
    (obj->*(method_info->class_method.method))(env, retval); \
} \
static void compiletimeBind(kscript::ExprParser* expr) { \
    kscript::ExprClassInfo* info = expr->addClass(L##class_name, \
                                           cpp_baseclass::staticGetClassName().c_str(), \
                                           cpp_class::getShadowObject(), \
                                           object_factory_func); \
} \
struct BoundMemberContainer : public kscript::ShadowObject { \
    std::map<std::wstring, BoundMemberInfo*> members; \
    ~BoundMemberContainer() { std::map<std::wstring, BoundMemberInfo*>::iterator it; \
        for (it = members.begin(); it != members.end(); ++it) delete it->second; \
    } \
    bool getMember(const std::wstring& name, kscript::Value* retval) { \
        std::map<std::wstring, BoundMemberInfo*>::iterator it = members.find(name); \
        if (it == members.end()) return false; \
        if (it->second->value.isUndefined()) { \
            if (it->second->is_static) \
                retval->setFunction((kscript::ExprBindFunc)it->second->func_ptr, it->second->param); \
                 else \
                retval->setFunction((kscript::ExprBindFunc)bind_func, (void*)it->second); \
            return true; \
        } \
        it->second->value.eval(NULL, retval); \
        return true; \
    } \
}; \
static kscript::ShadowObject* getShadowObject() { \
        static BoundMemberContainer m; \
        BoundMemberInfo* b = NULL; \
        if (m.members.empty()) {
    
    
#define BEGIN_KSCRIPT_CLASS(class_name, cpp_class) \
            _BEGIN_KSCRIPT_CLASS(class_name, cpp_class, kscript::ValueObject)

#define BEGIN_KSCRIPT_DERIVED_CLASS(class_name, cpp_class, cpp_baseclass) \
            _BEGIN_KSCRIPT_CLASS(class_name, cpp_class, cpp_baseclass)

#define KSCRIPT_METHOD(kscript_method, cpp_method) \
        m.members[L##kscript_method] = new BoundMemberInfo(L##kscript_method, &cpp_method, NULL, staticGetClassId(), false); \
        
#define KSCRIPT_STATIC_METHOD(kscript_method, cpp_method) \
        m.members[L##kscript_method] = new BoundMemberInfo(L##kscript_method, (kscript::ExprBindFunc)&cpp_method, NULL, staticGetClassId(), false); \
        
#define KSCRIPT_CONSTANT_INTEGER(kscript_method, val) \
        b = new BoundMemberInfo; \
        b->value.setInteger(val); \
        m.members[L##kscript_method] = b;
        
#define KSCRIPT_CONSTANT_DOUBLE(kscript_method, val) \
        b = new BoundMemberInfo; \
        b->value.setDouble(val); \
        m.members[L##kscript_method] = b;

#define KSCRIPT_CONSTANT_STRING(kscript_method, val) \
        b = new BoundMemberInfo; \
        b->value.setString(L##val); \
        m.members[L##kscript_method] = b;
           
#define END_KSCRIPT_CLASS() \
        } \
        return &m; \
    } 

    


};



#endif

