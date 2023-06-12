/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Clean Utility
 * Author:   Benjamin I. Williams
 * Created:
 *
 */


#include "stdafx.h"
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <comdef.h>
#include "../paladin/paladin.h"
#include "../paladin/crc.h"
#include "kl/klib.h"


// -- our "generic" key (don't change) --
static unsigned char generic_key[] = { 0x42, 0x49, 0x98, 0xba, 0x22, 0x34, 0x32, 0x8e };

// -- our "fingerprint" key (don't change) --
static unsigned char fingerprint_key[] = { 0x22, 0xc9, 0x98, 0xb8, 0x32, 0x54, 0x22, 0x9e };


// these are defined here because the old
// VC6 SDK doesn't have them
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x1c
#endif


#ifndef CSIDL_PROFILE
#define CSIDL_PROFILE 0x28
#endif


const int regMachine = 0;
const int regUser = 1;
const int regClasses = 2;


bool deleteRegKey(int scope, LPCTSTR path, LPCTSTR val)
{
    LONG result;
    HKEY root;

    switch (scope)
    {
        case regMachine: root = HKEY_LOCAL_MACHINE; break;
        case regUser:    root = HKEY_CURRENT_USER; break;
        case regClasses: root = HKEY_CLASSES_ROOT; break;
        default:
            return false;
    }

    result = RegDeleteKey(root, path);

    return (result == ERROR_SUCCESS ? true : false);
}



static void appendPathPart(char* path, const char* part)
{
    if (*path == 0)
    {
        strcpy(path, part);
        return;
    }

#ifdef WIN32
    char slash = '\\';
#else
    char slash = '/';
#endif

    if (*(path + strlen(path) - 1) != slash)
    {
        char s[2];
        s[0] = slash;
        s[1] = 0;
        strcat(path, s);
    }
    
    while (*part == slash)
        part++;

    strcat(path, part);
}


void buildCLSIDPath(LPCTSTR path1, const GUID& guid, TCHAR* path)
{
    // this builds a fake path which looks like
    // it shouldn't be removed.  It is really used
    // as a fingerprint to identify if a package
    // has been installed or not
    
    wchar_t guidstr[255];
    _tcscpy(path, path1);
    _tcscat(path, _T("\\"));
    StringFromGUID2(guid, guidstr, 255);
    _tcscat(path, kl::tstr(guidstr));
    _tcscat(path, _T("\\Option"));
}


bool getSpecialFolderPath(int folder_id, TCHAR* path)
{
    LPITEMIDLIST pidl = NULL;
    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)))
        return false;
    if (!pidl)
        return false;
    SHGetPathFromIDList(pidl, path);
    LPMALLOC pIMalloc;
    SHGetMalloc(&pIMalloc);
    pIMalloc->Free(pidl);
    pIMalloc->Release();
    return true;
}

// {BAA05053-28EB-4376-9D54-B0AF89C15A98}
static const GUID guid_fp1 = 
{ 0xbaa05053, 0x28eb, 0x4376, { 0x9d, 0x54, 0xb0, 0xaf, 0x89, 0xc1, 0x5a, 0x98 } };

// {88E0A525-57F4-44ce-844E-4954FB1AEAE1}
static const GUID guid_fp2 = 
{ 0x88e0a525, 0x57f4, 0x44ce, { 0x84, 0x4e, 0x49, 0x54, 0xfb, 0x1a, 0xea, 0xe1 } };

// {552866D6-09F2-4a3c-8573-F6745B7F217E}
static const GUID guid_fp3 = 
{ 0x552866d6, 0x9f2, 0x4a3c, { 0x85, 0x73, 0xf6, 0x74, 0x5b, 0x7f, 0x21, 0x7e } };





void removeForAppTag(LPCTSTR company_name, LPCTSTR tag)
{
    char fingerprint1[255];
    char fingerprint2[255];
    char fingerprint3[255];
    
    paladin::paladin_int64_t ik;
    
    // first fingerprint string will be the crc32 of the eval tag
    unsigned long eval_tag_crc = paladin::crc32((unsigned char*)tag, (int)strlen(tag));
    sprintf(fingerprint1, "%08X", eval_tag_crc);

    // second fingerprint string will be an int64 suitably randomized
    // by an encryption key
    ik = eval_tag_crc;
    paladin::int64crypt(ik, generic_key, true);
    paladin::getStringFromCode(ik, fingerprint2, false);
    
    // second fingerprint string will be the same as the second
    // fingerprint, simply created with a different key
    ik = eval_tag_crc;
    paladin::int64crypt(ik, fingerprint_key, true);
    paladin::getStringFromCode(ik, fingerprint3, false);
    
    
    
    
    
    TCHAR path[512];
    UUID guid;

    // remove fingerprint from
    // x:\Documents and Settings\username\Application Data
    if (getSpecialFolderPath(CSIDL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(fingerprint1));
        remove(path);
    }
    
    // remove fingerprint from
    // x:\Documents and Settings\username\Local Settings\Application Data
    if (getSpecialFolderPath(CSIDL_LOCAL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(fingerprint2));
        remove(path);
    }
    
    // remove fingerprint from
    // x:\Documents and Settings\username
    if (getSpecialFolderPath(CSIDL_PROFILE, path))
    {
        appendPathPart(path, kl::tstr(fingerprint3));
        remove(path);
    }


    // remove registry fingerprints
    TCHAR base_path[255];
    _sntprintf(base_path, 255, _T("Software\\%s\\Security"),
                    (TCHAR*)kl::tstr(company_name));
    memcpy(&guid, &guid_fp1, sizeof(UUID));
    guid.Data2 = (unsigned short)((eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(base_path, guid, path);
    deleteRegKey(regUser, path, _T(""));

    memcpy(&guid, &guid_fp2, sizeof(UUID));
    guid.Data2 = (unsigned short)((eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    deleteRegKey(regUser, path, _T(""));

    memcpy(&guid, &guid_fp3, sizeof(UUID));
    guid.Data2 = (unsigned short)((eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    deleteRegKey(regUser, path, _T(""));
}


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("palclean <company_name> <app_tag>\n");
        return 0;
    }

    TCHAR company_name[255];
    TCHAR app_id[255];
    _tcscpy(company_name, kl::tstr(argv[1]));
    _tcscpy(app_id, kl::tstr(argv[2]));
    
    
    int i;
    for (i = 0; i < 100; ++i)
    {
        TCHAR buf[255];
        _stprintf(buf, _T("%sTMP%d"), app_id, i);
        
        removeForAppTag(company_name, buf);
    }

    removeForAppTag(company_name, app_id);

    printf("Cleaned '%s' '%s' application tag.\n", company_name, app_id);
    return 0;
}

