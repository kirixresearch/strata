/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-03-27
 *
 */


#include "scripthost.h"
#include "native.h"
#include <kl/string.h>
#include <kl/portable.h>

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif



namespace scripthost
{


static void* openLibrary(const std::wstring& filename)
{
#ifdef WIN32
    return (void*)LoadLibraryW(filename.c_str());
#else
    std::string asc_path = kl::tostring(filename);
    return (void*)dlopen(asc_path.c_str(), RTLD_LAZY);
#endif
}

static void closeLibrary(void* handle)
{
#ifdef WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

static void* getProcAddress(void* handle, const char* func_name)
{
#ifdef WIN32
    return GetProcAddress((HMODULE)handle, func_name);
#else
    return (void*)dlsym(handle, func_name);
#endif
}


// (CLASS) NativeModule
// Category: System
// Description: A class that provides the ability to load and access shared 
//     libraries such as DLLs, SOs, and DYLIBs.
// Remarks: The NativeModule class provides the ability to access shared libraries
//     such as DLLs, SOs, and DYLIBs.
//
// Property(NativeType.Char):      Type indicator for a single-byte character ('C' char type)
// Property(NativeType.Byte):      Type indicator for a single byte ('C' unsigned char type)
// Property(NativeType.Word):      Type indicator for a 16-bit value ('C' short int type)
// Property(NativeType.Int32):     Type indicator for a 16-bit value ('C' int type)
// Property(NativeType.Int64):     Type indicator for a 64-bit value ('C' long long type)
// Property(NativeType.Float):     Type indicator for a 32-bit floating point value ('C' float type)
// Property(NativeType.Double):    Type indicator for a 64-bit floating point value ('C' double type)
// Property(NativeType.String8):   Type indicator for an 8-bit character string ('C' char* type)
// Property(NativeType.String16):  Type indicator for an 16-bit character string ('C' unsigned short* type)
// Property(NativeType.String32):  Type indicator for an 32-bit character string ('C' unsigned long* type)
// Property(NativeCall.Default):   Use the platform's default calling conventions
// Property(NativeCall.CDecl):     Use C calling conventions
// Property(NativeCall.StdCall):   Use stdcall calling conventions


NativeModule::NativeModule()
{
    m_handle = NULL;
}

NativeModule::~NativeModule()
{
    if (m_handle)
    {
        closeLibrary(m_handle);
    }
}


// (CONSTRUCTOR) NativeModule.constructor
// Description: Creates a NativeModule object referencing a shared library
//
// Syntax: NativeModule(filename : String)
//
// Remarks: Creates a new NativeModule object that references a shared library.
//     If the module is valid and loads properly, functions can then be loaded
//     from the module for invocation.
//
// Param(filename): The filename of the dll, so, or dylib file to load


void NativeModule::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() > 0)
    {
        std::wstring filename = env->getParam(0)->getString();
        m_handle = openLibrary(filename);
    }
}



// (METHOD) NativeModule.isOk
// Description: Determines if a shared library loaded properly
//
// Syntax: function NativeModule.isOk() : Boolean
//
// Remarks: Determines whether the object is attached to a valid shared library
//     and that the library loaded properly.
//
// Returns: True if the shared library is loaded, false otherwise


void NativeModule::isOk(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_handle ? true : false);
}


// (METHOD) NativeModule.getFunction
//
// Description: Load a function from the shared library for invocation
//
// Syntax: static function NativeModule.getFunction(function_name : String) : Integer
//
// Remarks: Calling getFunction loads a function from a shared library for
//     invocation.  The function's parameters, return type, and calling
//     convention must be specified.
//
// Param(function_name): The name of the function as it appears in the shared library
// Returns: A callable function object

void NativeModule::getFunction(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_handle)
    {
        retval->setNull();
        return;
    }
    
    if (env->getParamCount()< 3)
    {
        retval->setNull();
        return;
    }
    
    int convention = env->getParam(0)->getInteger();
    int retval_type = env->getParam(1)->getInteger();
    std::string func_name = kl::tostring(env->getParam(2)->getString());
    std::string func_name2;
    std::vector<int> params;
    
    unsigned int p;
    for (p = 3; p < env->m_param_count; ++p)
    {
        params.push_back(env->getParam(p)->getInteger());
    }
    
    
    if (convention == NativeCall::StdCall)
    {
        // NativeCall can also look like this _Function@4
        // where 4 is the number of stack bytes for the
        // parameters
        int stack_bytes = 0;
        int i, param_count = (int)params.size();
        for (i = 0; i < param_count; ++i)
        {
            switch (params[i])
            {
                default:
                    stack_bytes += 4;
                    break;
                case NativeType::Double:
                    stack_bytes += 8;
                    break;
            }
        }
        
        func_name2 = "_";
        func_name2 += func_name;
        func_name2 += "@";

        char buf[255];
        snprintf(buf, 255, "%d", stack_bytes);
        buf[254] = 0;

        func_name2 += buf;
    }
    
    
    
    void* proc = getProcAddress(m_handle, func_name.c_str());
    if (!proc)
    {
        if (func_name2.length() > 0)
            proc = getProcAddress(m_handle, func_name2.c_str());
    
        if (!proc)
        {
            retval->setNull();
            return;
        }
    }
    
    
    NativeCall* call = NativeCall::createObject(env);
    call->m_convention = convention;
    call->m_module = this;
    call->m_module->baseRef();
    call->m_proc = proc;
    call->m_retval_type = retval_type;
    call->m_param_count = (int)params.size();
    call->m_params = new NativeCallParam[params.size()+1];
    for (p = 0; p < params.size(); ++p)
        call->m_params[p].type = params[p];
    
    retval->setObject(call);
}




NativeCall::NativeCall()
{
    m_param_count = 0;
    m_module = NULL;
    m_proc = NULL;
}

NativeCall::~NativeCall()
{
    if (m_module)
    {
        m_module->baseUnref();
    }
}

void NativeCall::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{

}



void NativeCall::invoke(kscript::ExprEnv* env, kscript::Value* retval)
{
/*
    if (env->m_param_count != m_param_count)
    {
        retval->setNull();
        return;
    }

    int stack_bytes = 0;
    
    wchar_t* wstr;
    int slen;
    
    // place items in call param array
    int i, param_count = env->m_param_count;
    for (i = 0; i < param_count; ++i)
    {
        m_params[i].str = NULL;
        switch (m_params[i].type)
        {
            case NativeType::String16:
                m_params[i].value = (long)env->m_eval_params[i]->getString();
                stack_bytes += 4;
                break;

            case NativeType::String8:
                wstr = env->m_eval_params[i]->getString();
                if (env->m_eval_params[i]->isString())
                    slen = env->m_eval_params[i]->getDataLen();
                     else
                    slen = wcslen(wstr);
                m_params[i].str = new char[slen+1];
                strcpy(m_params[i].str, kl::tostring(wstr).c_str());
                m_params[i].value = (long)m_params[i].str;
                stack_bytes += 4;
                break;
            
            case NativeType::Double:
                m_params[i].dvalue = env->m_eval_params[i]->getDouble();
                stack_bytes += 8;
                break;
                
            case NativeType::Float:
                m_params[i].dvalue = env->m_eval_params[i]->getDouble();
                stack_bytes += 4;
                break;

            case NativeType::Char:
            case NativeType::Byte:
            case NativeType::Word:
            case NativeType::Int32:
            case NativeType::Int64:
            default:
                m_params[i].value = env->m_eval_params[i]->getInteger();
                stack_bytes += 4;
                break;
        }

    }



    long val;
    double dbl;
    float flt;
    int iscdecl = 0;
    int isretdbl = 0;
    int isretflt = 0;
    int ret_eax = 0;
    double ret_dbl = 0.0;
    double ret_flt = 0.0;
    
    void* proc = m_proc;
    if (m_convention == NativeCall::CDecl)
        iscdecl = 1;
    if (m_retval_type == NativeType::Double)
        isretdbl = 1;
    if (m_retval_type == NativeType::Float)
        isretflt = 1;
        
    for (i = param_count-1; i >= 0; --i)
    {
        if (m_params[i].type == NativeType::Double)
        {
            dbl = m_params[i].dvalue;

            #ifdef _MSC_VER
                _asm {
                sub esp, 8;
                fld dbl;
                fstp qword ptr [esp];
                }
            #else
                asm volatile("subl $8, %esp");
                asm volatile("fldl %0"::"g"(dbl));
                asm volatile("fstpl (%esp)");
            #endif
        }
         else if (m_params[i].type == NativeType::Float)
        {
            flt = m_params[i].dvalue;
            
            #ifdef _MSC_VER
                _asm {
                sub esp, 4;
                fld flt;
                fstp dword ptr [esp];
                }
            #else
                asm volatile("subl $4, %esp");
                asm volatile("flds %0"::"m"(flt));
                asm volatile("fstps (%esp)");
            #endif

        } 
         else
        {
            // add function call parameter to the stack
            val = m_params[i].value;
            
            #ifdef _MSC_VER
                _asm push val;
            #else
#if __SIZEOF_LONG__ == 4
                asm volatile("pushl %0" :: "g"(val));
#endif
#if __SIZEOF_LONG__ == 8
                asm volatile("pushq %0" :: "g"(val));
#endif
            #endif
        }
    }
    
    #ifdef _MSC_VER
        _asm call proc
        _asm mov ret_eax, eax
    #else
        asm volatile(
                 "call *%1\n"
                 "movl %%eax, %0\n"
                 : "=r"(ret_eax) : "g"(proc),"g"(stack_bytes) : "eax");
    #endif
    
    if (iscdecl == 1)
    {
        // call is cdecl -- clean up stack
        #ifdef _MSC_VER
            _asm add esp,stack_bytes;
        #else
            asm volatile("add %0,%%esp" :: "r"(stack_bytes) : "cc");
        #endif
    }

    if (isretdbl == 1)
    {
        // if the return type is a double, pop it off the stack
        #ifdef _MSC_VER
            _asm fstp ret_dbl;
        #else
            asm volatile("fstpl %0" : "=m"(ret_dbl));
        #endif
    }  
     else if (isretflt == 1)
    {
        // if the return type is a double, pop it off the stack
        #ifdef _MSC_VER
            _asm fstp ret_flt;
        #else
            asm volatile("fstps %0" : "=m"(ret_flt));
        #endif
    }

    
    // free any memory allocated
    for (i = 0; i < param_count; ++i)
    {
        if (m_params[i].str)
            delete[] m_params[i].str;
    }
    
    
    // process return value
    
    switch (m_retval_type)
    {
        case NativeType::Void:
            retval->setUndefined();
            break;
            
        case NativeType::Char:
        case NativeType::Byte:
        case NativeType::Word:
        case NativeType::Int32:
            retval->setInteger(ret_eax);
            break;
        
        case NativeType::Double:
            retval->setDouble(ret_dbl);
            break;
            
        case NativeType::Float:
            retval->setDouble(ret_flt);
            break;
    }
*/
}

void NativeCall::toFunction(kscript::ExprEnv* env, kscript::Value* retval)
{
    static kscript::Value invoke_func;
    if (invoke_func.isUndefined())
    {
        invoke_func.setValue(env->getThis()->getMember(L"invoke"));
    }
    
    retval->setValue(&invoke_func);
}




}; // namespace scripthost
