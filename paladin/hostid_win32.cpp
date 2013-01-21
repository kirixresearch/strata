/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-25
 *
 */


#include <windows.h>
#include <tchar.h>
#include "paladin.h"
#include "crc.h"

#if _MSC_VER < 1300
#define KEY_WOW64_64KEY 0x100
#endif


namespace paladin
{


static unsigned char generic_key[] = { 0x42, 0x49, 0x98, 0xba, 0x22, 0x34, 0x32, 0x8e };


// getHostId() returns a 48-bit unique host id

sitecode_t getHostId()
{
    sitecode_t s = 0;


    // get volume serial number for main volume
    TCHAR volname[255];
    TCHAR fsname[255];
    DWORD volser = 0;
    DWORD maxcomplen = 0;
    DWORD fsflags = 0;
    GetVolumeInformation(_T("C:\\"), volname, 255,
                         &volser, &maxcomplen, &fsflags,
                         fsname, 255);

    s = volser;
    
    // get windows installation id

    TCHAR val[255];
    LONG result;
    HKEY hkey;
    DWORD dtype;
    DWORD dlen;
    
    val[0] = 0;

    
    OSVERSIONINFO vi;
    memset(&vi, 0, sizeof(OSVERSIONINFO));
    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&vi);
    
    // only use KEY_WOW64_64KEY on newer versions of windows
    if (vi.dwMajorVersion >= 6)  
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                   0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hkey);

        if (result == ERROR_SUCCESS)
        {
            dtype = REG_SZ;
            dlen = 254;
            result = RegQueryValueEx(hkey, _T("ProductId"),
                                     0, &dtype, (LPBYTE)val, &dlen);
            if (result != ERROR_SUCCESS)
            {
                val[0] = 0;
            }

            RegCloseKey(hkey);
        }
         else
        {
            val[0] = 0;
        }
    }


    if (val[0] == 0)
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
                   0, KEY_QUERY_VALUE, &hkey);

        if (result == ERROR_SUCCESS)
        {
            dtype = REG_SZ;
            dlen = 254;
            result = RegQueryValueEx(hkey, _T("ProductId"),
                                     0, &dtype, (LPBYTE)val, &dlen);
            if (result != ERROR_SUCCESS)
            {
                val[0] = 0;
            }

            RegCloseKey(hkey);
        }
         else
        {
            val[0] = 0;
        }
    }



    // if it failed to find the key, try the older location for this value

    if (val[0] == 0)
    {
        result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                   _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"),
                   0, KEY_QUERY_VALUE, &hkey);

        if (result == ERROR_SUCCESS)
        {
            dtype = REG_SZ;
            dlen = 254;
            result = RegQueryValueEx(hkey, _T("ProductId"),
                                     0, &dtype, (LPBYTE)val, &dlen);
            if (result != ERROR_SUCCESS)
            {
                val[0] = 0;
            }

            RegCloseKey(hkey);
        }
         else
        {
            val[0] = 0;
        }
    }


    if (val[0] == 0)
    {
        return s;
    }



    //  convert key to ascii
    char ascval[255];
    int len = _tcslen(val);
    int i;
    for (i = 0; i < len; ++i)
    {
        ascval[i] = (char)val[i];
        ascval[i+1] = 0;
    }


    // compile final host id
    sitecode_t idcrc = crc32((unsigned char*)ascval, len);

    s <<= 32;
    s |= idcrc;

    // just to mix the two numbers together
    int64crypt(s, generic_key, true);

    // make it 48-bits

#ifdef _MSC_VER
    s &= 0xffffffffffff;
#else
    s &= 0xffffffffffffLL;
#endif

    return s;
}



};


