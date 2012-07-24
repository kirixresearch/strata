/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#ifndef __APP_SCRIPTNATIVE_H
#define __APP_SCRIPTNATIVE_H


class NativeType : public kscript::ValueObject
{
public:

    BEGIN_KSCRIPT_CLASS("NativeType", NativeType)

        KSCRIPT_METHOD("constructor", NativeType::constructor)

        KSCRIPT_CONSTANT_INTEGER("Char",       Char)
        KSCRIPT_CONSTANT_INTEGER("Byte",       Byte)
        KSCRIPT_CONSTANT_INTEGER("Word",       Word)
        KSCRIPT_CONSTANT_INTEGER("Int32",      Int32)
        KSCRIPT_CONSTANT_INTEGER("Int64",      Int64)
        KSCRIPT_CONSTANT_INTEGER("Float",      Float)
        KSCRIPT_CONSTANT_INTEGER("Double",     Double)
        KSCRIPT_CONSTANT_INTEGER("String8",    String8)
        KSCRIPT_CONSTANT_INTEGER("String16",   String16)
        KSCRIPT_CONSTANT_INTEGER("String32",   String32)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Void = 1,
        Char = 2,
        Byte = 3,
        Word = 4,
        Int32 = 5,
        Int64 = 6,
        Float = 7,
        Double = 8,
        String8 = 9,
        String16 = 10,
        String32 = 11
    };
    
public:    

    void constructor(kscript::ExprEnv* env, kscript::Value* retval) { }
};




class NativeModule : public kscript::ValueObject
{
    BEGIN_KSCRIPT_CLASS("NativeModule", NativeModule)
        KSCRIPT_METHOD("constructor", NativeModule::constructor)
        KSCRIPT_METHOD("isOk", NativeModule::isOk)
        KSCRIPT_METHOD("getFunction", NativeModule::getFunction)
    END_KSCRIPT_CLASS()

public:

    NativeModule();
    ~NativeModule();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void isOk(kscript::ExprEnv* env, kscript::Value* retval);
    void getFunction(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    void* m_handle;
};



class NativeCallParam
{
public:
    int type;
    char* str;
    
    union
    {
        long value;
        double dvalue;
    };
};

class NativeCall : public kscript::ValueObject
{
friend class NativeModule;

public:

    BEGIN_KSCRIPT_CLASS("NativeCall", NativeCall)

        KSCRIPT_METHOD("constructor", NativeCall::constructor)
        KSCRIPT_METHOD("invoke", NativeCall::invoke)
        KSCRIPT_METHOD("toFunction", NativeCall::toFunction)
        
        KSCRIPT_CONSTANT_INTEGER("Default",    Default)
        KSCRIPT_CONSTANT_INTEGER("CDecl",      CDecl)
        KSCRIPT_CONSTANT_INTEGER("StdCall",    StdCall)

    END_KSCRIPT_CLASS()

public:

    enum
    {
        Default = 0,
        CDecl = 1,
        StdCall = 2
    };

public:

    NativeCall();
    ~NativeCall();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void invoke(kscript::ExprEnv* env, kscript::Value* retval);
    void toFunction(kscript::ExprEnv* env, kscript::Value* retval);
    
private:

    NativeModule* m_module;
    void* m_proc;
    int m_convention;
    NativeCallParam* m_params;
    int m_param_count;
    int m_retval_type;
};




#endif
