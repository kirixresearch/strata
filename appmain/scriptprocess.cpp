/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams; Aaron L. Williams
 * Created:  2009-01-23
 *
 */
 

#include "appmain.h"
#include "scripthost.h"
#include "scriptprocess.h"
#include "scriptgui.h"
#include <wx/settings.h>
#include <kl/thread.h>


// (CLASS) Process
// Category: System
// Description: A class that provides access to a system process
// Remarks: The Process class provides functionality for
//     interacting with a system process.

Process::Process()
{
    m_wxprocess = NULL;
    m_process_id = 0;
    m_running = false;
    m_redirect = false;
}

Process::~Process()
{
    if (m_wxprocess)
        m_wxprocess->setProcessParent(NULL);
}

void Process::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
    initComponent(env);
    
    getMember(L"finished")->setObject(Event::createObject(env));
}


// static
void Process::start(kscript::ExprEnv* env, void* param, kscript::Value* retval)
{
    wxLogNull log;
    
    kscript::Value* val_this = env->getThis();
    if (val_this->isObject() && val_this->getObject()->isKindOf(Process::staticGetClassId()))
    {
        // the non-static version of this function
        Process* pThis = static_cast<Process*>(val_this->getObject());
        
        // create the process
        pThis->m_wxprocess = new ProcessInternal;
        if (!pThis->m_wxprocess)
            return;

        // set the process parent and turn on process
        // redirection so we can capture input/output
        pThis->m_wxprocess->setProcessParent(pThis);
        
        if (pThis->m_redirect)
            pThis->m_wxprocess->Redirect();

        // execute the command
        wxString process_command = env->getParam(0)->getString();
        
        pThis->m_process_id = wxExecute(process_command, wxEXEC_ASYNC, pThis->m_wxprocess);
        pThis->m_running = (pThis->m_process_id == 0 ? false : true);
    }
     else
    {
        ::wxExecute(env->getParam(0)->getString(), wxEXEC_ASYNC);
    }
}



class ProcessMainThreadStarter : public wxEvtHandler
{
    
public:
 
    ProcessMainThreadStarter(kscript::ExprEnv* env,
                             kscript::Value* retval,
                             bool* done_flag)
    {
        m_env = env;
        m_retval = retval;
        m_done_flag = done_flag;
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        Process::start(m_env, NULL, m_retval);
        *m_done_flag = true;
        
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
        return true;
    }
    
private:

    Process* m_process;
    kscript::ExprEnv* m_env;
    kscript::Value* m_retval;
    bool* m_done_flag;
};


void Process::startAndWait(kscript::ExprEnv* env, void*, kscript::Value* retval)
{   
    kscript::Value* old_this = NULL;
    kscript::Value new_this;
    
    Process* pThis = NULL;
    kscript::Value* val_this = env->getThis();
    if (val_this->isObject() && val_this->getObject()->isKindOf(Process::staticGetClassId()))
    {
        pThis = static_cast<Process*>(val_this->getObject());
    }
     else
    {
        pThis = Process::createObject(env);
        new_this.setObject(pThis);
        old_this = env->getThis();
        env->setThis(&new_this);
    }
    
    bool done = false;
    ProcessMainThreadStarter* p = new ProcessMainThreadStarter(env, retval, &done);
    
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 19000);
    ::wxPostEvent(p, evt);

    while (!done)
        kl::Thread::sleep(50);
        
    pThis->waitForExit(env, retval);
    
    if (old_this)
        env->setThis(old_this);
}




class ProcessFetchOutput : public wxEvtHandler
{
    
public:
 
    ProcessFetchOutput(Process* process)
    {
        m_process = process;
    }
    
    bool ProcessEvent(wxEvent& evt)
    {
        m_process->transferOutput();
        if (!wxPendingDelete.Member(this))
            wxPendingDelete.Append(this);
        return true;
    }
    
private:
    Process* m_process;
};



// (METHOD) Process.waitForExit
//
// Description: Waits for the process to terminate
//
// Syntax: function Process.waitForExit()
//
// Remarks: After starting a process with start(), calling waitForExit()
//     will block the calling thread until the process has completed
//     execution.


void Process::waitForExit(kscript::ExprEnv* env, kscript::Value* retval)
{
    this->baseRef();
    
    while (m_running)
    {
        ProcessFetchOutput* f = new ProcessFetchOutput(this);
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 19000);
        ::wxPostEvent(f, evt);
        
        kl::Thread::sleep(1000);
    }
    
    this->baseUnref();
}



// (METHOD) Process.isRunning
//
// Description: Determines whether the process is running
//
// Syntax: function Process.isRunning() : Boolean
//
// Remarks: After starting a process with start(), calling isRunning()
//     can be used to determine whether the process is still running
//     or not


void Process::isRunning(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setBoolean(m_running);
}


// (METHOD) Process.getStandardOutput
//
// Description: Retrieves the stdout stream of the process
//
// Syntax: function Process.getStandardError() : ProcessOutputStream
//
// Remarks: getStandardOutput() returns a ProcessOutputStream which allows
//     the caller to retrieve the |stdout| contents of a process.


void Process::getStandardOutput(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_redirect)
    {
        retval->setNull();
        return;
    }
    
    ProcessOutputStream* obj = ProcessOutputStream::createObject(env);
    obj->m_process = this;
    obj->m_type = ProcessOutputStream::typeStdout;
    obj->m_lines = &m_stdout_lines;
    obj->m_process->baseRef();
    retval->setObject(obj);
}

// (METHOD) Process.getStandardError
//
// Description: Retrieves the stderr stream of the process
//
// Syntax: function Process.getStandardError() : ProcessOutputStream
//
// Remarks: getStandardError() returns a ProcessOutputStream which allows
//     the caller to retrieve the |stderr| output of a process.

void Process::getStandardError(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_redirect)
    {
        retval->setNull();
        return;
    }
    
    ProcessOutputStream* obj = ProcessOutputStream::createObject(env);
    obj->m_process = this;
    obj->m_type = ProcessOutputStream::typeStderr;
    obj->m_lines = &m_stderr_lines;
    obj->m_process->baseRef();
    retval->setObject(obj);
}


// (METHOD) Process.setRedirect
//
// Description: Puts process object in redirect mode
//
// Syntax: function Process.setRedirect(redirect : Boolean)
//
// Remarks: Calling this method puts the process object in redirect mode.
//     Redirct mode is useful for capturing the standard output and standard
//     error output text of the process.  A call to setRedirect() is necessary
//     in order to use the getStandardOutput() and/or getStandardError() methods.
//     setRedirect() must be called before start() is invoked.


void Process::setRedirect(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (env->getParamCount() == 0)
        m_redirect = true;
         else
        m_redirect = env->getParam(0)->getBoolean();
}


// (METHOD) Process.kill
//
// Description: Stops a system process
//
// Syntax: function Process.kill() : Boolean
//
// Remarks: This function kills the process associated with this object.
//
// Returns: True if the process is successfully stopped, and false otherwise.

void Process::kill(kscript::ExprEnv* env, kscript::Value* retval)
{
    // TODO: add flags to determine whether or not to kill
    // this process or process tree (child processes)

    // default return value
    retval->setBoolean(false);

    if (!m_wxprocess)
        return;

    // kill the process; if the process couldn't be killed,
    // or something else went wrong, then return
    int kill_result;
    kill_result = wxProcess::Kill(m_wxprocess->GetPid());

    if (kill_result != wxKILL_OK)
        return;

    // the process was stopped sucessfully, so set
    // the return value to true
    retval->setBoolean(true);
}


// (METHOD) Process.getPid
//
// Description: Returns the process id associated with a process.
//
// Syntax: function Process.getPid() : Integer
//
// Remarks: This function returns the process id for the process
//     represented by this object.  If no process is associated
//     with the object, the function returns 0.
//
// Returns: Returns the process id for the process represented by
//     this object.

void Process::getPid(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setInteger(m_process_id);
}

void Process::transferOutput()
{
    if (!m_wxprocess)
        return;
        
    if (!m_running)
        return;

    wxInputStream* stream = m_wxprocess->GetInputStream();
    if (stream)
    {
        wxTextInputStream text_stream(*stream);
        
        while (stream->CanRead())
        {
            wxString str = text_stream.ReadLine();
            m_stdout_lines.push_back(towstr(str));
        }
    }
    
    stream = m_wxprocess->GetErrorStream();
    if (stream)
    {
        wxTextInputStream text_stream(*stream);
        
        while (stream->CanRead())
        {
            wxString str = text_stream.ReadLine();
            m_stderr_lines.push_back(towstr(str));
        }
    }
}


void Process::processFinished()
{
    m_running = false;
    
    // reset the process id
    m_process_id = 0;

    // fire the finished event
    invokeJsEvent(L"finished");
}




#ifdef __WXMSW__


struct PROCESS_MEMORY_COUNTERS_STRUCT
{
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
};

typedef BOOL (WINAPI *GetProcessMemoryInfoFunc)(HANDLE, PROCESS_MEMORY_COUNTERS_STRUCT*, DWORD);

class ProcessInfo
{
public:
    ProcessInfo()
    {
        HINSTANCE h = LoadLibrary(_T("PSAPI.DLL"));
        m_pGetProcessMemoryInfo = (GetProcessMemoryInfoFunc)GetProcAddress(h, "GetProcessMemoryInfo");
    }
    
    bool fillMemoryStruct(int pid, PROCESS_MEMORY_COUNTERS_STRUCT* pmc)
    {
        if (!m_pGetProcessMemoryInfo)
            return false;
            
        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
        if (!h)
            return false;

        memset(pmc, 0, sizeof(PROCESS_MEMORY_COUNTERS_STRUCT));
        pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS_STRUCT);
    
        if (!m_pGetProcessMemoryInfo(h, pmc, sizeof(PROCESS_MEMORY_COUNTERS_STRUCT)))
        {
            CloseHandle(h);
            return false;
        }
        
        return true;
    }
    
    bool getMemoryInfo(int pid, __int64* total, __int64* working)
    {
        PROCESS_MEMORY_COUNTERS_STRUCT pmc;
        if (!fillMemoryStruct(pid, &pmc))
            return false;
            
        *total = (pmc.WorkingSetSize + pmc.PagefileUsage);
        *working = pmc.WorkingSetSize;
        return true;
    }
    
private:

    GetProcessMemoryInfoFunc m_pGetProcessMemoryInfo;
};

#else

class ProcessInfo
{
public:

    bool getMemoryInfo(int pid, long long* total, long long* working)
    {
        return false;
    }
};

#endif

ProcessInfo g_process_info;


void Process::getMemoryInfo(kscript::ExprEnv* env, kscript::Value* retval)
{
    retval->setNull();
    
    long long total, working;
    
    if (!g_process_info.getMemoryInfo(m_process_id, &total, &working))
        return;
    
    retval->setObject(kscript::Object::createObject(env));
    retval->createMember(L"total")->setDouble((double)total);
    retval->createMember(L"working")->setDouble((double)working);
}







// (CLASS) ProcessOutputStream
// Category: System
// Description: Provides access to the output of processes
// Remarks: The ProcessOutputStream class allows the caller
//     to access the stdout and stderr output streams of a
//     process.

ProcessOutputStream::ProcessOutputStream()
{
    m_process = NULL;
    m_lines = NULL;
    m_type = typeStdout;
}

ProcessOutputStream::~ProcessOutputStream()
{
    if (m_process)
        m_process->baseUnref();
}


void ProcessOutputStream::constructor(kscript::ExprEnv* env, kscript::Value* retval)
{
}


// (METHOD) ProcessOutputStream.eof
//
// Description: Determines if the end of the stream has been reached
//
// Syntax: function ProcessOutputStream.eof() : Boolean
//
// Remarks: When reading the stream, it is often necessary to
//     determine when the end has been encountered.  Calling eof()
//     returns true if the process has terminated and there are no
//     lines left in the input buffer.  If the process is still
//     running, eof() will always return true.  If the process has
//     terminated, eof() will return |true| if there are any pending
//     lines in the buffer.
//
// Returns: A boolean value indicating whether the end of the stream
//     has been reached.

void ProcessOutputStream::eof(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (!m_process)
    {
        retval->setBoolean(true);
        return;
    }
    
    if (m_process->m_running)
    {
        retval->setBoolean(false);
        return;
    }
    
    retval->setBoolean(m_lines->size() > 0 ? false : true);
}



// (METHOD) ProcessOutputStream.readLine
//
// Description: Read a single line from a text file
//
// Syntax: function ProcessOutputStream.readLine() : String
//
// Remarks: Reads a single line from a process output stream.  The file
//     pointer is automatically advanced to the next line.  A string
//     containing the line read is returned, and does not include the
//     carriage return or line-feed character.  If data is not yet available
//     for reading, or an end-of-file (eof) condition has been reached,
//     null is returned.  See eof() for more details.
//
// Returns: A string containing the line read from the input stream.
//     If an error or end-of-file condition was encountered, null is
//     returned.

void ProcessOutputStream::readLine(kscript::ExprEnv* env, kscript::Value* retval)
{
    if (m_process->m_wxprocess)
    {
        wxInputStream* stream = NULL;
        if (m_type == typeStdout)
            stream = m_process->m_wxprocess->GetInputStream();
             else
            stream = m_process->m_wxprocess->GetErrorStream();
            
        if (stream)
        {
            wxTextInputStream text_stream(*stream);
            
            if (!stream->CanRead())
            {
                retval->setNull();
                return;
            }
            
            wxString str = text_stream.ReadLine();
            retval->setString(towstr(str));
            return;
        }
    }
     else
    {
        if (!m_lines || m_lines->size() == 0)
        {
            retval->setNull();
            return;
        }
        
        wxString& str = m_lines->front();
        retval->setString(towstr(str));
        m_lines->pop_front();
    }

}



// ProcessInternal helper class

ProcessInternal::ProcessInternal()
{
    m_parent = NULL;
}

ProcessInternal::~ProcessInternal()
{
}

void ProcessInternal::OnTerminate(int pid, int status)
{
    if (!m_parent)
        return;
        
    m_parent->transferOutput();

    // if the parent pointer exists, reset the pointer 
    // in the parent
    if (m_parent)
        m_parent->m_wxprocess = NULL;

    // notify the parent that the process
    // is finished
    if (m_parent)
        m_parent->processFinished();

    delete this;
}

void ProcessInternal::setProcessParent(Process* parent)
{
    m_parent = parent;
}


