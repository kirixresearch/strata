/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XCM (eXtensible Component Model)
 * Author:   Benjamin I. Williams
 * Created:  2002-03-02
 *
 */


#ifndef __XCM_XCMWIN32_H
#define __XCM_XCMWIN32_H


#ifdef WIN32


#ifndef WINVER
#if (__VISUALC__ >= 1300)
#define WINVER 0x0600
#else
#define WINVER 0x0400
#endif
#endif


#define WIN32_EXTRA_LEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE




#define NOMINMAX

// if we are compiling with mingw 

/*
#ifndef _MSC_VER
#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__
#endif
*/

//#include <windef.h>
//#include <winnt.h>
//#include <winbase.h>


#include <cstdarg>
#include <windows.h>


#undef AppendMenu
#undef CallWindowProc
#undef ChangeDisplaySettings
#undef ChangeMenu
#undef CharLower
#undef CharLowerBuff
#undef CharNext
#undef CharNextEx
#undef CharPrev
#undef CharPrevEx
#undef CharToOem
#undef CharToOemBuff
#undef CharUpper
#undef CharUpperBuff
#undef CopyAcceleratorTable
#undef CreateAcceleratorTable
#undef CreateDesktop
#undef CreateDialog
#undef CreateDialogIndirect
#undef CreateDialogIndirectParam
#undef CreateDialogParam
#undef CreateMDIWindow
#undef CreateWindow
#undef CreateWindowEx
#undef CreateWindowStation
#undef DefDlgProc
#undef DefFrameProc
#undef DefMDIChildProc
#undef DefWindowProc
#undef DialogBox
#undef DialogBoxIndirect
#undef DialogBoxIndirectParam
#undef DialogBoxParam
#undef DispatchMessage
#undef DlgDirList
#undef DlgDirListComboBox
#undef DlgDirSelectComboBoxEx
#undef DlgDirSelectEx
#undef DrawState
#undef DrawText
#undef DrawTextEx
#undef EnumDesktops
#undef EnumDisplaySettings
#undef EnumProps
#undef EnumPropsEx
#undef EnumWindowStations
#undef FindWindow
#undef FindWindowEx
#undef GetClassInfo
#undef GetClassInfoEx
#undef GetClassLong
#undef GetClassName
#undef GetClipboardFormatName
#undef GetDlgItemText
#undef GetKeyboardLayoutName
#undef GetKeyNameText
#undef GetMenuItemInfo
#undef GetMenuString
#undef GetMessage
#undef GetMonitorInfo
#undef GetProp
#undef GetTabbedTextExtent
#undef GetUserObjectInformation
#undef GetWindowLong
#undef GetWindowLongPtr
#undef GetWindowText
#undef GetWindowTextLength
#undef GetAltTabInfo
#undef GetWindowModuleFileName
#undef GrayString
#undef InsertMenu
#undef InsertMenuItem
#undef IsCharAlpha
#undef IsCharAlphaNumeric
#undef IsCharLower
#undef IsCharUpper
#undef IsDialogMessage
#undef LoadAccelerators
#undef LoadBitmap
#undef LoadCursor
#undef LoadIcon
#undef LoadCursorFromFile
#undef LoadImage 
#undef LoadKeyboardLayout
#undef LoadMenu
#undef LoadMenuIndirect
#undef LoadString
#undef MapVirtualKey
#undef MapVirtualKeyEx
#undef MessageBox
#undef MessageBoxEx
#undef MessageBoxIndirect
#undef ModifyMenu
#undef OemToChar
#undef OemToCharBuff
#undef OpenDesktop
#undef OpenWindowStation
#undef PeekMessage
#undef PostAppMessage
#undef PostMessage
#undef PostThreadMessage
#undef RegisterClass
#undef RegisterClassEx
#undef RegisterClipboardFormat
#undef RegisterWindowMessage
#undef RemoveProp
#undef SendDlgItemMessage
#undef SendMessage
#undef SendMessageCallback
#undef SendMessageTimeout
#undef SendNotifyMessage
#undef SetClassLong
#undef SetDlgItemText
#undef SetMenuItemInfo
#undef SetProp
#undef SetUserObjectInformation
#undef SetWindowLong 
#undef SetWindowLongPtr
#undef SetWindowsHook
#undef SetWindowsHookEx
#undef SetWindowText
#undef SystemParametersInfo
#undef TabbedTextOut
#undef TranslateAccelerator
#undef UnregisterClass
#undef VkKeyScan
#undef VkKeyScanEx
#undef WinHelp


#endif


/*
#ifdef WIN32
#ifndef _WINBASE_
#ifndef _WINNT_H

#ifdef _MSC_VER
#define XCM_DLLIMPORT   _declspec(dllimport)
#else
#define XCM_DLLIMPORT
#endif

#define CONST const
typedef long LONG;
typedef void* HANDLE;
typedef unsigned long DWORD;
typedef int WINBOOL,*PWINBOOL,*LPWINBOOL;
typedef WINBOOL BOOL;
typedef unsigned char BYTE;
typedef BOOL *PBOOL,*LPBOOL;
typedef unsigned short WORD;
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef BYTE *PBYTE,*LPBYTE;
typedef int *PINT,*LPINT;
typedef WORD *PWORD,*LPWORD;
typedef long *LPLONG;
typedef DWORD *PDWORD,*LPDWORD;
typedef void *PVOID,*LPVOID;
typedef CONST void *PCVOID,*LPCVOID;
typedef int INT;
typedef unsigned int UINT,*PUINT,*LPUINT;


typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY,*PLIST_ENTRY;
typedef struct _CRITICAL_SECTION_DEBUG {
    WORD Type;
    WORD CreatorBackTraceIndex;
    struct _CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Spare [2];
} CRITICAL_SECTION_DEBUG,*PCRITICAL_SECTION_DEBUG;
typedef struct _CRITICAL_SECTION {
    PCRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD SpinCount;
} CRITICAL_SECTION,*PCRITICAL_SECTION,*LPCRITICAL_SECTION;


extern "C"
{
XCM_DLLIMPORT void* _stdcall HeapAlloc(void*, unsigned long, unsigned long);
XCM_DLLIMPORT void* _stdcall HeapReAlloc(void*, unsigned long, void*, unsigned long);
XCM_DLLIMPORT int _stdcall HeapFree(void*, unsigned long, void*);
XCM_DLLIMPORT void* _stdcall GetProcessHeap(void);

XCM_DLLIMPORT long _stdcall InterlockedIncrement(long* lpAddend);
XCM_DLLIMPORT long _stdcall InterlockedDecrement(long* lpAddend);
};

#undef XCM_DLLIMPORT

#endif  // #ifndef _WINNT_H_
#endif  // #ifndef _WINBASE_
#endif  // #ifdef WIN32
*/


#endif

