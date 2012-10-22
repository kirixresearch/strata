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
#include <vector>
#include <kl/string.h>
#include "gpasvc.h"


HANDLE g_app_process = NULL;
HANDLE g_app_thread = NULL;
DWORD g_last_hang = 0;



extern const TCHAR* g_service_appexe;
TCHAR g_service_appexe_fullpath[MAX_PATH];
void SvcReportEvent(LPTSTR szFunction);


static void logInfo(const char* msg)
{
#ifdef _DEBUG
    printf(msg);
#endif
}

bool DoesFileExist(LPCTSTR file_path)
{
    HANDLE h;
    WIN32_FIND_DATA fd;
    h = FindFirstFile(file_path, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}



static void DetermineFullFilename()
{
    g_service_appexe_fullpath[0] = 0;

    TCHAR buf[MAX_PATH];
    
    GetModuleFileName(NULL, buf, MAX_PATH-1);
    TCHAR* slash = _tcsrchr(buf, _T('\\'));
    if (slash)
        *slash = 0;

    _tcscat(buf, _T("\\"));
    _tcscat(buf, g_service_appexe);
    
    if (!DoesFileExist(buf))
    {
        slash = _tcsrchr(buf, _T('\\'));
        if (slash)
            *slash = 0;
        _tcscat(buf, _T("\\"));
        _tcscat(buf, _T("appmain.exe"));
    }
    
    _tcscpy(g_service_appexe_fullpath, buf);
}


bool IsApplicationRunning()
{
    if (!g_app_process)
        return false;

    DWORD exit_code = 0;
    GetExitCodeProcess(g_app_process, &exit_code);

    return (exit_code == STILL_ACTIVE ? true : false);
}

DWORD RunApplicationProcess(LPCTSTR _cmd_line)
{
    if (g_app_process)
    {
        CloseHandle(g_app_process);
        CloseHandle(g_app_thread);
        g_app_process = NULL;
        g_app_thread = NULL;
    }

    DWORD result = 0;
    
    TCHAR* cmd_line = _tcsdup(_cmd_line);
    
    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_info;
    memset(&startup_info, 0, sizeof(STARTUPINFO));
    memset(&process_info, 0, sizeof(PROCESS_INFORMATION));
    startup_info.cb = sizeof(STARTUPINFO);

    if (CreateProcess(NULL,
                      cmd_line,
                      0, 0, false,
                      0, // CREATE_DEFAULT_ERROR_MODE,
                      0, 0,
                      &startup_info,
                      &process_info) != false)
    {
        result = ERROR_SUCCESS;
    }
     else
    {
        result = GetLastError();
    }
    
    free(cmd_line);

    g_app_process = process_info.hProcess;
    g_app_thread = process_info.hThread;

    return result;
}


bool StopApplicationProcess()
{
    if (!g_app_process)
        return false;

    bool success = (FALSE == TerminateProcess(g_app_process, (UINT)-1)) ? false : true;

    if (success)
    {
        CloseHandle(g_app_thread);
        CloseHandle(g_app_process);
        g_app_process = NULL;
        g_app_thread = NULL;
    }

    return success;
}


static BOOL CALLBACK EnumThreadWindowsCallback(HWND hwnd, LPARAM lParam)
{
    HWND* ptr = (HWND*)lParam;

    // get the first visible window
    if (::IsWindowVisible(hwnd) && ::IsWindowEnabled(hwnd))
        *ptr = hwnd;

    return TRUE;
}

HWND GetApplicationMainWindow()
{
    HWND hwnd = NULL;   
    EnumThreadWindows(GetThreadId(g_app_thread), EnumThreadWindowsCallback, (LPARAM)&hwnd);
    return hwnd;
}

bool IsApplicationHanging()
{
    HWND hwnd = GetApplicationMainWindow();
    if (!hwnd)
        return false;

    // try five times to determine whether the
    // application is hung

    size_t i, hung_count = 0;
    for (i = 0; i < 5; i++)
    {
        if (!IsHungAppWindow(hwnd))
            return false;
        logInfo("Hanging...\n");
        ::Sleep(2000);
    }

    return true;
}


void Pulse()
{
    static int counter = 0;

    if (!IsApplicationRunning())
    {
        logInfo("Process not running... running it now\n");

        TCHAR cmdline[MAX_PATH];
        _tcscpy(cmdline, g_service_appexe_fullpath);
        _tcscat(cmdline, _T(" "));
        _tcscat(cmdline, g_service_cmdline);

        RunApplicationProcess(cmdline);
        Sleep(10000);
    }


    // once every five minutes, check to see if the application is hanging
    if (++counter == 30)
    {
        counter = 0;

        if (IsApplicationHanging())
        {
            logInfo("Application is hanging... terminating.\n");
            StopApplicationProcess();
        }
    }
}


DWORD ServiceExecutionThread(DWORD* param)
{
    DetermineFullFilename();
    
    while (1)
    {
        Pulse();
        
        Sleep(10000);
    }
}

