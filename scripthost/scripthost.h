/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-07-29
 *
 */


#ifndef H_SCRIPTHOST_SCRIPTHOST_H
#define H_SCRIPTHOST_SCRIPTHOST_H


#include <queue>
#include <kl/thread.h>
#include <kl/xcm.h>

#include "../kscript/kscript.h"
#include "../kscript/jsobject.h"
#include "../kscript/jsarray.h"


namespace scripthost
{



class ScriptHost;


// TODO: Please note that Event and ScriptHostBase aren't yet entirely ported from appmain

class ScriptHostBase : public kscript::ValueObject
{
public:

    ScriptHostBase();
    void initComponent(kscript::ExprEnv* env);
    
   // Application* getApp();
    ScriptHost* getScriptHost();
    
    size_t getJsEventSinkCount(const std::wstring& evt);
    
    size_t invokeJsEvent(const std::wstring& evt,
                         kscript::Value* event_args = NULL,
                         unsigned int event_flags = 0);
                         
    size_t invokeJsEvent(kscript::Value* evt,
                         kscript::Value* event_args = NULL,
                         unsigned int event_flags = 0);
      
private:

   // ApplicationWeakReference m_app;
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

    kl::mutex m_obj_mutex;
};


}; // namespace scripthost


#endif
