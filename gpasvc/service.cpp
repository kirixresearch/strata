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
#include "gpasvc.h"


DWORD g_app_process_id = 0;
DWORD g_last_hang = 0;


typedef DWORD (WINAPI *PFN_GetProcessImageFileName)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);
typedef DWORD (WINAPI *PFN_GetVolumePathName)(LPCTSTR lpszFileName, LPTSTR lpszVolumePathName, DWORD cchBufferLength);
typedef DWORD (WINAPI *PFN_GetModuleFileNameEx)(HANDLE, HMODULE, LPTSTR, DWORD);

PFN_GetProcessImageFileName pGetProcessImageFileName = NULL;
PFN_GetVolumePathName       pGetVolumePathName = NULL;
PFN_GetModuleFileNameEx     pGetModuleFileNameEx = NULL;


extern const TCHAR* g_service_appexe;
TCHAR g_service_appexe_fullpath[MAX_PATH];


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

static bool LoadNeededDllFunctions()
{
    HMODULE hpsapi = LoadLibraryA("psapi.dll");
    HMODULE hKernel32 = LoadLibraryA("kernel32.dll");
    
    pGetModuleFileNameEx = (PFN_GetModuleFileNameEx)GetProcAddress(hpsapi, "GetModuleFileNameExW");
    pGetProcessImageFileName = (PFN_GetProcessImageFileName)GetProcAddress(hpsapi, "GetProcessImageFileNameW");
    pGetVolumePathName = (PFN_GetVolumePathName)GetProcAddress(hKernel32, "GetVolumePathNameW");
    if ((pGetVolumePathName != NULL && pGetProcessImageFileName != NULL) ||
         pGetModuleFileNameEx != NULL)
    {
        // we have what we need...
        return true;
    }
     else
    {
        printf("Couldn't get procedure address...");
        return false;
    }
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




bool GetCommandLine(LPTSTR cmd_line, size_t maxlen)
{

    return true;
}


bool GetProcessImageName(DWORD pid, LPTSTR cmdline, size_t maxlen)
{
    if (maxlen == 0)
        return false;
    if (cmdline)
        cmdline[0] = 0;
    
    
    HANDLE h = NULL;
    bool success = false;
    
    if (pGetProcessImageFileName && pGetVolumePathName)
    {
        h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!h)
            return false;

        TCHAR image_name[MAX_PATH], path[MAX_PATH];
        pGetProcessImageFileName(h, image_name, MAX_PATH);
        
        pGetVolumePathName(image_name, path, MAX_PATH-1);

        size_t len = _tcslen(path);
        if (len > 0 && path[len-1] != '\\')
            _tcscat(path, _T("\\"));

        // find third slash in image_name
        TCHAR* sl;
        sl = _tcschr(image_name, '\\');
        if (sl)
            sl = _tcschr(sl+1, '\\');
        if (sl)
            sl = _tcschr(sl+1, '\\');
        if (sl)
            _tcscat(path, sl+1);
        _tcsncpy(cmdline, path, maxlen-1);
        cmdline[maxlen-1] = 0;
    }
     else if (pGetModuleFileNameEx)
    {
        h = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);
        if (!h)
            return false;

        if (0 == pGetModuleFileNameEx(h, (HMODULE)0, cmdline, maxlen))
        {
            CloseHandle(h);
            return false;
        }
    }
     else
    {
        CloseHandle(h);
        return false;
    }


    
    CloseHandle(h);
    
    return true;
}


DWORD RunProcess(LPCTSTR _cmd_line, DWORD* process_id)
{
    DWORD result = 0;
    
    TCHAR* cmd_line = _tcsdup(_cmd_line);
    
    STARTUPINFO startup_info;
    PROCESS_INFORMATION process_info;
    memset(&startup_info, 0, sizeof(STARTUPINFO));
    memset(&process_info, 0, sizeof(PROCESS_INFORMATION));
    startup_info.cb = sizeof(STARTUPINFO);

    if (process_id)
        *process_id = 0;
    
    if (CreateProcess(NULL,
                      cmd_line,
                      0, 0, false,
                      0, // CREATE_DEFAULT_ERROR_MODE,
                      0, 0,
                      &startup_info,
                      &process_info) != false)
    {
        result = ERROR_SUCCESS;
        if (process_id)
            *process_id = process_info.dwProcessId;
    }
     else
    {
        result = GetLastError();
    }
    
    free(cmd_line);

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    return result;
}


bool TerminateProcessById(DWORD process_id)
{
    bool success = false;
    
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
    
    if (h)
    {
        success = (FALSE == TerminateProcess(h, (UINT)-1)) ? false : true;

        CloseHandle(h);
    }
    
    return success;
}



// IsCorrectProcess() returns 'true' if the pid specified
// refers to the application being monitored.  Returns 'false'
// if the pid is either not running or reflects another process
// which is not the application

bool IsCorrectProcess(DWORD pid)
{
    TCHAR path[MAX_PATH];
    
    if (!GetProcessImageName(pid, path, MAX_PATH-1))
        return false;
    
    _tcsupr(path);
    
    if (0 != lstrcmpi(path, g_service_appexe))
        return true;
    
    return false;
}



class Info
{
public:
    std::vector<HWND> windows;
};


static BOOL CALLBACK EnumAllProcessWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (g_app_process_id == 0)
        return FALSE;
    
    Info* info = (Info*)lParam;
    
    DWORD pid;      // process id
    DWORD tid;      // thread id
    
    tid = GetWindowThreadProcessId(hwnd, &pid);
    
    if (pid == g_app_process_id)
    {
        if (::IsWindowVisible(hwnd) && ::IsWindowEnabled(hwnd))
        {
            info->windows.push_back(hwnd);
        }
    }

    return TRUE;
}



static BOOL CALLBACK EnumAllCrashWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (g_app_process_id == 0)
        return FALSE;
        
    Info* info = (Info*)lParam;
    
    if (::IsWindowVisible(hwnd) && ::IsWindowEnabled(hwnd))
    {    
        DWORD pid;      // process id
        DWORD tid;      // thread id
        
        tid = GetWindowThreadProcessId(hwnd, &pid);
        
        TCHAR path[MAX_PATH];
        if (GetProcessImageName(pid, path, MAX_PATH-1))
        {
            _tcsupr(path);
            if (NULL != _tcsstr(path, _T("DWWIN.EXE")) ||
                NULL != _tcsstr(path, _T("DRWTSN32.EXE")))
            {
                TCHAR clsname[255];
                GetClassName(hwnd, clsname, 254);
                if (0 == _tcscmp(clsname, _T("#32770")))
                    info->windows.push_back(hwnd);
            }
        }
    }
    
    return TRUE;
}


void CloseAllCrashWindows()
{
    Info info;
    EnumWindows(EnumAllCrashWindowsProc, (LPARAM)&info);

    std::vector<HWND>::iterator it;
    for (it = info.windows.begin(); it != info.windows.end(); ++it)
    {
        ::SendMessage(*it, WM_CLOSE, 0, 0);
    }
}

BOOL IsHung(HWND hwnd)
{
    DWORD d = 1;
    

    LRESULT res = SendMessageTimeout(hwnd,
                                     WM_NULL,
                                     0,
                                     0,
                                     SMTO_NORMAL,
                                     1000,
                                     &d);
    if (res == 0)
        return true;
        
    if (d != 0)
        return true;
        
    return false;
}


bool IsProcessHanging()
{
    Info info;
    EnumWindows(EnumAllProcessWindowsProc, (LPARAM)&info);
    
    size_t hung_window_count = 0;
    std::vector<HWND>::iterator it;
    for (it = info.windows.begin(); it != info.windows.end(); ++it)
    {
        if (IsHung(*it))
        {
            ++hung_window_count;
        }
    }
    
    
    if (hung_window_count > 0 && hung_window_count == info.windows.size())
    {
        return true;
    }
    
    return false;
}



void CheckProcessHealth()
{
    if (!g_app_process_id)
        return;
    
    size_t i, hang_count;
    
    for (hang_count = 0; hang_count < 10; ++hang_count)
    {
        if (!IsProcessHanging())
            return;
        
        printf("Process is hanging (%d)...\n", (hang_count+1));
        Sleep(6000);
    }
    
    printf("Process has been hanging for at least a minute...\n");
    
    // it's been a minute, and the application is still hanging;
    // shut it down and restart it
    
    bool succeeded = false;
    for (i = 0; i < 3; ++i)
    {
        if (TerminateProcessById(g_app_process_id))
        {
            succeeded = true;
            break;
        }
    }
    
    if (!succeeded)
    {
        printf("Process could not be terminated.\n");
        return;
    }
    
    CloseAllCrashWindows();
    
    // restart it
    g_app_process_id = 0;
}


void Pulse()
{
    if (g_app_process_id != 0)
    {
        if (IsCorrectProcess(g_app_process_id))
        {
            // we are monitoring the correct process, make sure
            // it's healthy
            
            CheckProcessHealth();
        }
         else
        {
            // we are monitoring a process, but it either is not
            // running, or is not the process we wanted to be monitoring
            
            // cause the application to be (re)started
            g_app_process_id = 0;
        }
    }



    if (g_app_process_id == 0)
    {
        printf("Process not running... running it now\n");
        RunProcess(g_service_appexe_fullpath, &g_app_process_id);
        Sleep(10000);
    }
    
    
    CheckProcessHealth();
}



DWORD ServiceExecutionThread(DWORD* param)
{
    LoadNeededDllFunctions();
    DetermineFullFilename();
    
    while (1)
    {
        Pulse();
        
        Sleep(10000);
    }
}


void StopApplicationProcess()
{
    TerminateProcessById(g_app_process_id);
}


