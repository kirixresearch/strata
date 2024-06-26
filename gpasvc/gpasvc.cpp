/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  General Purpose Application Service
 * Author:   Benjamin I. Williams
 * Created:  2008-12-31
 *
 */


#define _CRT_SECURE_NO_WARNINGS 1


#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <kl/json.h>
#include <kl/string.h>
#include <kl/file.h>



const TCHAR* g_service_id = _T("SampleSvc");
const TCHAR* g_service_description = _T("General Purpose Application Service");
const TCHAR* g_service_appexe = _T("sample.exe");
const TCHAR* g_service_cmdline = _T("");


/* sample config file

{
    "service_id":           "SampleSvc",
    "service_description":  "Sample Service",
    "service_appexe":       "sample.exe"
    "service_cmdline":      ""
}

*/



SERVICE_STATUS g_svc_status;
SERVICE_STATUS_HANDLE g_svc_status_handle;
HANDLE g_svc_stop_event = NULL;

DWORD ServiceExecutionThread(DWORD* param);
bool StopApplicationProcess();






void SvcInstall()
{
    SC_HANDLE scmgr;
    SC_HANDLE svc;
    TCHAR path[MAX_PATH];

    if (!GetModuleFileName(NULL, path, MAX_PATH))
    {
        printf("Cannot install service (%u)\n", GetLastError());
        return;
    }


    // open scm database
    scmgr = OpenSCManager(
                    NULL,                    // local computer
                    NULL,                    // ServicesActive database
                    SC_MANAGER_ALL_ACCESS);  // full access rights
 
    if (!scmgr)
    {
        printf("OpenSCManager failed (%u)\n", GetLastError());
        return;
    }

    // create the service

    svc = CreateService(
                    scmgr,                     // SCM database
                    g_service_id,              // name of service
                    g_service_description,     // service name to display
                    SERVICE_ALL_ACCESS,        // desired access
                    SERVICE_WIN32_OWN_PROCESS, // service type
                    SERVICE_DEMAND_START,      // start type
                    SERVICE_ERROR_NORMAL,      // error control type
                    path,                      // path to service's binary
                    NULL,                      // no load ordering group
                    NULL,                      // no tag identifier
                    NULL,                      // no dependencies
                    NULL,                      // LocalSystem account
                    NULL);                     // no password
 
    if (!svc)
    {
        printf("CreateService failed (%u)\n", GetLastError());
        CloseServiceHandle(scmgr);
        return;
    }
     else
    {
        printf("Service installed successfully\n");
    }


    CloseServiceHandle(svc);
    CloseServiceHandle(scmgr);
}


void SvcUninstall()
{
    SC_HANDLE schService;
    SC_HANDLE schSCManager;
    SERVICE_STATUS ssStatus;
    
    schSCManager = OpenSCManager(
                        NULL,
                        NULL,
                        SC_MANAGER_ALL_ACCESS);
                        
    if (schSCManager)
    {
        schService = OpenService(schSCManager, g_service_id, SERVICE_ALL_ACCESS);

        if (schService)
        {
            // try to stop the service
            if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus))
            {
                _tprintf(_T("Stopping service '%s'..."), g_service_description);
                Sleep(2000);

                while (QueryServiceStatus(schService, &ssStatus))
                {
                    if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
                        Sleep(1000);
                         else
                        break;
                }
                _tprintf(_T("done.\n"));
            }

            // now remove the service
            if (DeleteService(schService))
                _tprintf(_T("'%s' service removed.\n"), g_service_description);
                 else
                _tprintf(_T("DeleteService failed (%d)\n"), GetLastError());

            CloseServiceHandle(schService);
        }
         else
        {
            _tprintf(_T("Service '%s' could not be opened -- it's likely not installed.\n"), g_service_description);
        }
    }

    CloseServiceHandle(schSCManager);
}


void ReportSvcStatus(DWORD dwCurrentState,
                     DWORD dwWin32ExitCode,
                     DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;


    g_svc_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_svc_status.dwServiceSpecificExitCode = 0;
    g_svc_status.dwCurrentState = dwCurrentState;
    g_svc_status.dwWin32ExitCode = dwWin32ExitCode;
    g_svc_status.dwWaitHint = dwWaitHint;


    if (dwCurrentState == SERVICE_START_PENDING)
        g_svc_status.dwControlsAccepted = 0;
         else
        g_svc_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;


    if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
        g_svc_status.dwCheckPoint = 0;
         else
        g_svc_status.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus(g_svc_status_handle, &g_svc_status);
}



void SvcReportEvent(LPTSTR message)
{
/*
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, _T("Application"));

    if (NULL != hEventSource)
    {
        lpszStrings[0] = g_service_id;
        lpszStrings[1] = message;

        ReportEvent(hEventSource,              // event log handle
                    EVENTLOG_INFORMATION_TYPE, // event type
                    0,                         // event category
                    SVC_ERROR,                 // event identifier
                    NULL,                      // no security identifier
                    2,                         // size of lpszStrings array
                    0,                         // no binary data
                    lpszStrings,               // array of strings
                    NULL);                     // no binary data

        DeregisterEventSource(hEventSource);
    }
    */
}





void WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
    // handle the requested control code

    switch (dwCtrl)
    {
        case SERVICE_CONTROL_STOP:
            ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

            StopApplicationProcess();
            
            // signal the service to stop
            SetEvent(g_svc_stop_event);
            return;

        case SERVICE_CONTROL_INTERROGATE:
            // fall through to send current status
            break; 

        default: 
            break;
    } 

    ReportSvcStatus(g_svc_status.dwCurrentState, NO_ERROR, 0);
}





void SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
    // create a stop event which will be triggered when
    // the service should be stopped

    g_svc_stop_event = CreateEvent(
                         NULL,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         NULL);   // no name

    if (g_svc_stop_event == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }




    DWORD id;
    HANDLE thread_handle;

    thread_handle = CreateThread(0, 0,
                                (LPTHREAD_START_ROUTINE)ServiceExecutionThread,
                                0, 0, &id);
    if (thread_handle == 0)
    {
        ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }


    // service thread successfully created, set our status to 'running'
    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // wait for the service to be stopped
    WaitForSingleObject(g_svc_stop_event, INFINITE);

    // report that the service stopped
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}



void WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // register the handler function for the service

    g_svc_status_handle = RegisterServiceCtrlHandler(
                                            g_service_id,
                                            SvcCtrlHandler);

    if (!g_svc_status_handle)
    {
        SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
        return;
    }

    // report initial status to the SCM
    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // initialize the service
    SvcInit(dwArgc, lpszArgv);
}


bool LoadConfig()
{
    TCHAR buf[512];

    GetModuleFileName(NULL, buf, 511);
    TCHAR* slash = _tcsrchr(buf, '\\');
    if (slash)
        *slash = 0;


    std::wstring config_file = kl::towstring(buf) + L"\\gpasvc.conf";

    std::wstring contents = xf_get_file_contents(config_file);
    if (contents.length() == 0)
        return false;

    kl::JsonNode node;
    if (!node.fromString(contents))
        return false; // bad parse


    kl::JsonNode node_service_id = node["service_id"];
    kl::JsonNode node_service_description = node["service_description"];
    kl::JsonNode node_service_appexe = node["service_appexe"];
    kl::JsonNode node_service_cmdline = node["service_cmdline"];

    if (node_service_id.isNull() || node_service_description.isNull() || node_service_appexe.isNull() || node_service_cmdline.isNull())
        return false;

    
    TCHAR* service_id = new TCHAR[node_service_id.getString().length() + 1];
    TCHAR* service_description = new TCHAR[node_service_description.getString().length() + 1];
    TCHAR* service_appexe = new TCHAR[node_service_appexe.getString().length() + 1];
    TCHAR* service_cmdline = new TCHAR[node_service_cmdline.getString().length() + 1];

    _tcscpy(service_id, kl::tstr(node_service_id.getString()));
    _tcscpy(service_description, kl::tstr(node_service_description.getString()));
    _tcscpy(service_appexe, kl::tstr(node_service_appexe.getString()));
    _tcscpy(service_cmdline, kl::tstr(node_service_cmdline.getString()));

    g_service_id = service_id;
    g_service_description = service_description;
    g_service_appexe = service_appexe;
    g_service_cmdline = service_cmdline;

    return true;
}


void __cdecl _tmain(int argc, TCHAR *argv[])
{
    if (!LoadConfig())
    {
        printf("Could not load configuration file gpasvc.conf - File was either missing or was not in valid JSON format\n");
        return;
    }


    //ServiceExecutionThread(NULL);
    //return;
    
    // passing 'install' on the command line will register
    // this with the SCM database.  It will then appear in
    // the list of services in the control panel

    if (argc >= 2 && 0 == lstrcmpi(argv[1], _T("install")))
    {
        SvcInstall();
        return;
    }

    if (argc >= 2 && 0 == lstrcmpi(argv[1], _T("uninstall")))
    {
        SvcUninstall();
        return;
    }
    
    
    static TCHAR service_id[255];
    _tcscpy(service_id, g_service_id);

    SERVICE_TABLE_ENTRY DispatchTable[] =
    {
        { service_id, (LPSERVICE_MAIN_FUNCTION)SvcMain },
        { NULL, NULL }
    };
 

    // this call will block until the service is finished running
    if (!StartServiceCtrlDispatcher(DispatchTable))
    {
        SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
    }

    return;
}


