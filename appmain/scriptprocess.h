/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2009-01-23
 *
 */


#ifndef H_APP_SCRIPTPROCESS_H
#define H_APP_SCRIPTPROCESS_H


#include <wx/txtstrm.h>


// forward declaration for helper class
class ProcessInternal;

class Process : public ScriptHostBase
{
friend class ProcessInternal;
friend class ProcessOutputStream;

public:

    BEGIN_KSCRIPT_CLASS("Process", Process)
        KSCRIPT_GUI_METHOD("constructor", Process::constructor)
        KSCRIPT_STATIC_GUI_METHOD("start", Process::start)
        KSCRIPT_STATIC_METHOD("startAndWait", Process::startAndWait)
        KSCRIPT_METHOD("waitForExit", Process::waitForExit)
        KSCRIPT_GUI_METHOD("isRunning", Process::isRunning)
        KSCRIPT_GUI_METHOD("setRedirect", Process::setRedirect)
        KSCRIPT_GUI_METHOD("getStandardOutput", Process::getStandardOutput)
        KSCRIPT_GUI_METHOD("getStandardError", Process::getStandardError)
        KSCRIPT_GUI_METHOD("kill", Process::kill)
        KSCRIPT_GUI_METHOD("getPid", Process::getPid)
        KSCRIPT_METHOD("getMemoryInfo", Process::getMemoryInfo)
    END_KSCRIPT_CLASS()

public:

    Process();
    ~Process();

    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    static void start(kscript::ExprEnv* env, void*, kscript::Value* retval);
    static void startAndWait(kscript::ExprEnv* env, void*, kscript::Value* retval);
    void waitForExit(kscript::ExprEnv* env, kscript::Value* retval);
    void isRunning(kscript::ExprEnv* env, kscript::Value* retval);
    void getStandardOutput(kscript::ExprEnv* env, kscript::Value* retval);
    void getStandardError(kscript::ExprEnv* env, kscript::Value* retval);
    void setRedirect(kscript::ExprEnv* env, kscript::Value* retval);
    
    void kill(kscript::ExprEnv* env, kscript::Value* retval);
    void getPid(kscript::ExprEnv* env, kscript::Value* retval);
    void getMemoryInfo(kscript::ExprEnv* env, kscript::Value* retval);
    
    void transferOutput();
    void processFinished();

private:

    ProcessInternal* m_wxprocess;
    int m_process_id;
    bool m_running;
    bool m_redirect;
    
    std::list<wxString> m_stdout_lines;
    std::list<wxString> m_stderr_lines;
};




class ProcessOutputStream : public ScriptHostBase
{
friend class Process;

public:

    enum
    {
        typeStdout = 1,
        typeStderr = 2,
    };
    
private:

    BEGIN_KSCRIPT_CLASS("ProcessOutputStream", ProcessOutputStream)
        KSCRIPT_GUI_METHOD("constructor", ProcessOutputStream::constructor)
        KSCRIPT_GUI_METHOD("readLine", ProcessOutputStream::readLine)
        KSCRIPT_GUI_METHOD("eof", ProcessOutputStream::eof)
    END_KSCRIPT_CLASS()

public:

    ProcessOutputStream();
    ~ProcessOutputStream();
    
    void constructor(kscript::ExprEnv* env, kscript::Value* retval);
    void readLine(kscript::ExprEnv* env, kscript::Value* retval);
    void eof(kscript::ExprEnv* env, kscript::Value* retval);

private:

    Process* m_process;
    int m_type;
    std::list<wxString>* m_lines;
};



// helper class
class ProcessInternal : public wxProcess
{
public:

    ProcessInternal();
    virtual ~ProcessInternal();
    void setProcessParent(Process* parent);
    
    void OnTerminate(int pid, int status);
    
public:

    Process* m_parent;
};




#endif

