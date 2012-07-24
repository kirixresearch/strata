/*!
 *
 * Copyright (c) 2002-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2002-03-02
 *
 */


#ifndef __KL_WIN32_H
#define __KL_WIN32_H


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


#ifndef _X86_
#define _X86_
#endif


#define NOMINMAX


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


#endif