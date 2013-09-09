/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#ifndef __APP_SCRIPTHOST_H
#define __APP_SCRIPTHOST_H


#include <queue>
#include <kl/thread.h>
#include <xcm/xcm.h>

#include "../kscript/kscript.h"
#include "../kscript/jsobject.h"
#include "../kscript/jsarray.h"


class ScriptHost;


class ScriptHostBase : public kscript::ValueObject
{
public:

    ScriptHostBase();
    void initComponent(kscript::ExprEnv* env);
    
    ScriptHost* getScriptHost();
    
private:

    ScriptHost* m_script_host;
};



typedef void (*BindFunc)(kscript::ExprEnv* env,
                         void* param,
                         kscript::Value* retval);


class ScriptHostInclude
{
public:

    enum
    {
        includeFile,
        includeProjectFile
    };
    
    int type;
    std::wstring path;
};



class ScriptHostParams
{
public:

    kscript::Value print_function;
    std::map<std::wstring, kscript::Value*> global_vars;
};


class ScriptHost
{
public:

    ScriptHost();
    virtual ~ScriptHost();

    void setParams(ScriptHostParams* params);
    bool compilePackage();
    
    bool compile(const wchar_t* text);
    bool compile(const std::wstring& text);
    
    bool run();
    void cancel();

    std::wstring getErrorFile();
    std::wstring getErrorString();
    int getErrorLine();
    size_t getErrorOffset();
    int getErrorCode();
    kscript::Value* getRetval();
    
    void addValue(const std::wstring& var_name, kscript::Value& val);
    
    void addFunction(const std::wstring& func_name,
                     BindFunc func_ptr,
                     const std::wstring& formats);

    void addIncludePath(const ScriptHostInclude& inc);
    const std::vector<ScriptHostInclude>& getIncludePaths();
    
    void setStartupPath(const std::wstring& startup_path);
    std::wstring getStartupPath() const;
    
public:

    xcm::signal0 sigJobChanged;
    kscript::Value m_print_function;

private:

    static bool script_host_parse_hook(kscript::ExprParseHookInfo& info);
    
    bool getFullIncludePath(const std::wstring& include_path, ScriptHostInclude* result);

private:

    kscript::ExprParser* m_expr;
    std::vector<void*> m_to_delete;
    std::vector<ScriptHostInclude> m_include_paths;
    std::queue<ScriptHostInclude> m_include_contexts;
    kscript::Value* m_retval;
    std::wstring m_startup_path;
        
    int m_error_code;
    std::wstring m_error_file;
    std::wstring m_error_string;
    int m_error_line;
    size_t m_error_offset;

    xcm::mutex m_obj_mutex;
};



#endif
