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
#include "appmain.h"
#include "../kscript/kscript.h"
#include "../kscript/jsobject.h"
#include "../kscript/jsarray.h"
#include <kl/thread.h>

// class forwards

class ExtensionPkg;
class GuiMarshal;
class Application;
class SystemColors;
class SystemMetrics;
class ScriptHost;

extern GuiMarshal gui_marshal;




// ApplicationWeakReference is used for getting a pointer
// to the script's global Application class safely.  If the
// application had been destroyed, the weak reference will
// return NULL

class ApplicationWeakReference : public xcm::signal_sink
{
public:

    ApplicationWeakReference();
    
    void setApp(Application* app);
    Application* getApp();
    
private:
    
    void onAppDestructing();
    
private:
    
    Application* m_app;
};



class ScriptHostBase : public kscript::ValueObject
{
public:

    ScriptHostBase();
    void initComponent(kscript::ExprEnv* env);
    
    Application* getApp();
    ScriptHost* getScriptHost();
    
    size_t getJsEventSinkCount(const std::wstring& evt);
    
    size_t invokeJsEvent(const std::wstring& evt,
                         kscript::Value* event_args = NULL,
                         unsigned int event_flags = 0);
                         
    size_t invokeJsEvent(kscript::Value* evt,
                         kscript::Value* event_args = NULL,
                         unsigned int event_flags = 0);
    
    // this is a receptor for a wx event, which might need
    // to be transated into a js event.  Please note that
    // this method is _always_ executed in the main thread.            
    virtual void onEvent(wxEvent& event) { }
      
private:

    ApplicationWeakReference m_app;
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
    wxString path;
};


class GuiMarshalSync
{
public:
    wxMutex* mutex;
    wxCondition* condition;
};


class ScriptHostParams
{
public:

    kscript::Value print_function;
    std::map<std::wstring, kscript::Value*> global_vars;
};


class ScriptHost : public wxEvtHandler
{
    friend class GuiMarshal;
    
public:

    ScriptHost();
    virtual ~ScriptHost();

    void setParams(ScriptHostParams* params);
    void setPackage(ExtensionPkg* package); // takes ownership
    ExtensionPkg* getPackage() const;
    bool compilePackage();
    
    bool compile(const wchar_t* text);
    bool compile(const wxString& text);
    
    bool run();
    void cancel();

    wxString getErrorFile();
    wxString getErrorString();
    int getErrorLine();
    size_t getErrorOffset();
    int getErrorCode();
    kscript::Value* getRetval();
    
    void addValue(const std::wstring& var_name, kscript::Value& val);
    
    void addFunction(const std::wstring& func_name,
                     BindFunc func_ptr,
                     const std::wstring& formats);

    void addGuiFunction(const std::wstring& func_name,
                     BindFunc func_ptr,
                     const std::wstring& formats);
    
    void addIncludePath(const ScriptHostInclude& inc);
    const std::vector<ScriptHostInclude>& getIncludePaths();
    
    void setStartupPath(const wxString& startup_path);
    wxString getStartupPath() const;
    
public:

    void setCurrentJob(IJobPtr job);
    IJobPtr getCurrentJob();
    
    xcm::signal0 sigJobChanged;
    kscript::Value m_print_function;

private:

    GuiMarshalSync* pushGuiMarshalSync();
    void popGuiMarshalSync();

    static bool script_host_parse_hook(kscript::ExprParseHookInfo& info);
    
    bool getFullIncludePath(const wxString& include_path, ScriptHostInclude* result);
    
    void onGuiMarshal(wxCommandEvent& evt);
    void onTextEntry(wxCommandEvent& evt);

private:

    Application* m_script_app;
    kscript::ExprParser* m_expr;
    IJobPtr m_curjob;
    std::vector<void*> m_to_delete;
    std::vector<ScriptHostInclude> m_include_paths;
    std::queue<ScriptHostInclude> m_include_contexts;
    kscript::Value* m_retval;
    wxString m_startup_path;
    ExtensionPkg* m_pkg;
        
    int m_error_code;
    wxString m_error_file;
    wxString m_error_string;
    int m_error_line;
    size_t m_error_offset;

    std::vector<GuiMarshalSync*> m_gui_syncs;
    size_t m_gui_sync_pos;

    xcm::mutex m_obj_mutex;

    DECLARE_EVENT_TABLE()
};



// -- GuiMarshal stuff --

class GuiMarshal : public wxEvtHandler
{
public:

    struct GuiMarshalFunc
    {
        kscript::ExprBindFunc func;
        void* param;
    };

    GuiMarshal();
    ~GuiMarshal();
    
    static void func_guiMarshal(kscript::ExprEnv* env,
                                void* param,
                                kscript::Value* retval);

    bool ProcessEvent(wxEvent& _event);

    void addGuiFunction(kscript::ExprParser* expr,
                        const std::wstring& func_name,
                        kscript::ExprBindFunc func_ptr,
                        const std::wstring& formats,
                        void* param = NULL);
    
    void getRerouteInfo(kscript::ExprBindFunc func_ptr,
                        void* param,
                        kscript::ExprBindFunc* result_ptr,
                        void** result_param);
    
    void getRerouteInfo(kscript::ExprBindFunc func_ptr,
                        void* param,
                        kscript::Value* retval);

public:

    std::vector<GuiMarshalFunc*> m_to_delete;
    std::vector<kscript::BoundMemberInfoBase*> m_to_delete2;
};


#define KSCRIPT_GUI_METHOD(kscript_method, cpp_method) { \
            kscript::ExprBindFunc new_func; \
            void* new_param; \
            BoundMemberInfo* method = new BoundMemberInfo(L##kscript_method, \
                                                          &cpp_method, \
                                                          NULL, \
                                                          staticGetClassId(), \
                                                          false); \
            gui_marshal.m_to_delete2.push_back(method); \
            gui_marshal.getRerouteInfo(&bind_func, \
                                       (void*)method, \
                                       &new_func, \
                                       &new_param); \
            m.members[L##kscript_method] = new BoundMemberInfo(L##kscript_method, \
                                              new_func, \
                                              new_param, \
                                              staticGetClassId(), \
                                              false); \
        }
 

#define KSCRIPT_STATIC_GUI_METHOD(kscript_method, cpp_static_method) { \
            kscript::ExprBindFunc new_func; \
            void* new_param; \
            gui_marshal.getRerouteInfo(cpp_static_method, \
                                       NULL, \
                                       &new_func, \
                                       &new_param); \
            m.members[L##kscript_method] = new BoundMemberInfo(L##kscript_method, \
                                              new_func, \
                                              new_param, \
                                              staticGetClassId(), \
                                              false); \
        }
 
 
 


#define KSCRIPT_BEGIN_PROPERTY_MAP(cppclass, baseclass) \
struct GuiPropertyInfo \
{ \
    BoundClassMethod getter; \
    BoundClassMethod setter; \
    bool gui; \
    GuiPropertyInfo() { } \
    GuiPropertyInfo(BoundClassMethod _getter, BoundClassMethod _setter, bool _gui) \
            : getter(_getter), setter(_setter), gui(_gui) {} \
}; \
class GuiPropertyMarshal : public wxEvtHandler \
{ \
public: \
    bool ProcessEvent(wxEvent& evt) \
    { \
        if (set) \
            obj->setMember(name, value); \
        else \
            *retval = obj->getMember(name); \
        done = true; \
        delete this; \
        return true; \
    } \
    kscript::ValueObject* obj; \
    std::wstring name; \
    kscript::Value* value; \
    kscript::Value** retval; \
    bool done; \
    bool set; \
}; \
void setMember(const std::wstring& name, kscript::Value* value) \
{ \
    GuiPropertyInfo* method = lookupScriptProperty(name); \
    if (method) \
    { \
        if (method->gui && !kl::Thread::isMain()) \
        { \
            GuiPropertyMarshal* m = new GuiPropertyMarshal; \
            m->obj = this; \
            m->name = name; \
            m->value = value; \
            m->done = false; \
            m->set = true; \
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 19000); \
            ::wxPostEvent(m, evt); \
            while (!m->done) \
                kl::Thread::sleep(1); \
        } \
         else \
        { \
            kscript::ExprEnv env; \
            kscript::Value retval; \
            kscript::Value* params[1]; \
            params[0] = value; \
            env.m_eval_params = params; \
            env.m_param_count = 1; \
            (this->*(method->setter))(&env, &retval); \
            env.m_eval_params = NULL; \
            env.m_param_count = 0; \
        } \
    } \
     else \
    { \
        baseclass::setMember(name, value); \
    } \
} \
kscript::Value* getMember(const std::wstring& name) \
{ \
    GuiPropertyInfo* method = lookupScriptProperty(name); \
    if (method) \
    { \
        if (method->gui && !kl::Thread::isMain()) \
        { \
            kscript::Value* retval = NULL; \
            GuiPropertyMarshal* m = new GuiPropertyMarshal; \
            m->obj = this; \
            m->name = name; \
            m->done = false; \
            m->set = false; \
            m->retval = &retval; \
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 19000); \
            ::wxPostEvent(m, evt); \
            while (!m->done) \
                kl::Thread::sleep(1); \
            return retval; \
        } \
         else \
        { \
            kscript::ExprEnv env; \
            env.m_param_count = 0; \
            (this->*(method->getter))(&env, getValueReturn()); \
            return getValueReturn(); \
        } \
    } \
     else \
    { \
        return baseclass::getMember(name); \
    } \
} \
GuiPropertyInfo* lookupScriptProperty(const std::wstring& name) { \
    static std::map<std::wstring, GuiPropertyInfo> mymap; \
    if (mymap.empty()) { \
    
#define KSCRIPT_PROPERTY(name, getmethod, setmethod) \
        mymap[L##name] = GuiPropertyInfo(&getmethod, &setmethod, false); \

#define KSCRIPT_GUI_PROPERTY(name, getmethod, setmethod) \
        mymap[L##name] = GuiPropertyInfo(&getmethod, &setmethod, true); \
    
    
#define KSCRIPT_END_PROPERTY_MAP() \
    } \
    std::map<std::wstring, GuiPropertyInfo>::iterator it; \
    it = mymap.find(name); \
    if (it == mymap.end()) \
        return NULL; \
         else \
        return &(it->second); \
}

#endif
