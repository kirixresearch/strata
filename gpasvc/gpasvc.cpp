/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
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
#include "../build_config.h"


// default is APP_STRATA
#if !defined(APP_STRATA)
#define APP_STRATA 1
#endif


#if APP_STRATA == 1
const TCHAR* g_service_id = _T("KirixStrataSvc");
const TCHAR* g_service_desc = _T("Kirix Strata");
const TCHAR* g_service_appexe = _T("kstrata.exe");
#endif




SERVICE_STATUS g_svc_status;
SERVICE_STATUS_HANDLE g_svc_status_handle;
HANDLE g_svc_stop_event = NULL;

DWORD ServiceExecutionThread(DWORD* param);
void StopApplicationProcess();






void SvcInstall()
{
    SC_HANDLE scmgr;
    SC_HANDLE svc;
    TCHAR path[MAX_PATH];

    if (!GetModuleFileName(NULL, path, MAX_PATH))
    {
        printf("Cannot install service (%d)\n", GetLastError());
        return;
    }


    // open scm database
    scmgr = OpenSCManager(
                    NULL,                    // local computer
                    NULL,                    // ServicesActive database
                    SC_MANAGER_ALL_ACCESS);  // full access rights
 
    if (!scmgr)
    {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // create the service

    svc = CreateService(
                    scmgr,                     // SCM database
                    g_service_id,              // name of service
                    g_service_desc,            // service name to display
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
        printf("CreateService failed (%d)\n", GetLastError());
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
                _tprintf(_T("Stopping service '%s'..."), g_service_desc);
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
                _tprintf(_T("'%s' service removed.\n"), g_service_desc);
                 else
                _tprintf(_T("DeleteService failed (%d)\n"), GetLastError());

            CloseServiceHandle(schService);
        }
         else
        {
            _tprintf(_T("Service '%s' could not be opened -- it's likely not installed.\n"), g_service_desc);
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



void SvcReportEvent(LPTSTR szFunction)
{
/*
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, g_service_id);

    if (NULL != hEventSource)
    {
        StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

        lpszStrings[0] = g_service_id;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource,        // event log handle
                    EVENTLOG_ERROR_TYPE, // event type
                    0,                   // event category
                    SVC_ERROR,           // event identifier
                    NULL,                // no security identifier
                    2,                   // size of lpszStrings array
                    0,                   // no binary data
                    lpszStrings,         // array of strings
                    NULL);               // no binary data

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




void __cdecl _tmain(int argc, TCHAR *argv[])
{
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


