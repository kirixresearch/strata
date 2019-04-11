/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-30
 *
 */


#include <windows.h>
#include <shlwapi.h>
#include <ctime>
#include <vector>
#include "kl/file.h"
#include "kl/string.h"


std::wstring xf_get_win32_error_msg()
{
    LPVOID lpMsgBuf;
    
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR)&lpMsgBuf,
        0,
        NULL 
    );
    
    std::wstring result;
    
    result = kl::towstring((LPCTSTR)lpMsgBuf);
    
    
    LocalFree( lpMsgBuf );
    
    return result;
}



// WNetGetUniversalNameW requires the mpr.lib stub library
// This call is supported on all versions of Windows,
// but WNetGetUniversalNameW is only supported on unicode versions
#pragma comment(lib, "mpr.lib")



struct dir_info
{
    HANDLE m_handle;
    WIN32_FIND_DATA* m_first;
};


xf_dirhandle_t xf_opendir(const std::wstring& dir)
{
    if (dir.length() == 0)
        return NULL;

    bool mask_provided = false;
    int mask_pos = dir.find(L'*');
    if (mask_pos != -1)
        mask_provided = true;
    mask_pos = dir.find(L'?');
    if (mask_pos != -1)
        mask_provided = true;

    std::wstring final_dir;
    final_dir = dir;

    if (!mask_provided)
    {
        if (final_dir[final_dir.length()-1] == L'\\')
            final_dir += L"*.*";
             else
            final_dir += L"\\*.*";
    }

    dir_info* info = new dir_info;
    info->m_first = new WIN32_FIND_DATA;

#ifdef _UNICODE
    info->m_handle = FindFirstFile(final_dir.c_str(), info->m_first);
#else
    info->m_handle = FindFirstFile(kl::tstr(final_dir), info->m_first);
#endif

    if (info->m_handle == INVALID_HANDLE_VALUE)
    {
        delete info->m_first;
        delete info;
        return NULL;
    }
    return (xf_dirhandle_t)info;
}

bool xf_readdir(xf_dirhandle_t d, xf_direntry_t* entry)
{
    WIN32_FIND_DATA fd;
    dir_info* di = (dir_info*)d;
    if (!di)
    {
        return false;
    }

    if (di->m_first)
    {
        memcpy(&fd, di->m_first, sizeof(WIN32_FIND_DATA));
        delete di->m_first;
        di->m_first = NULL;
    }
     else
    {
        BOOL b = FindNextFile(di->m_handle, &fd);
        if (!b)
        {
            return false;
        }
    }

#ifdef _UNICODE
    entry->m_name = fd.cFileName;
#else
    entry->m_name = (wchar_t*)kl::tstr(fd.cFileName);
#endif

    entry->m_type = xfFileTypeNormal;
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        entry->m_type = xfFileTypeDirectory;
    }

    return true;
}

bool xf_closedir(xf_dirhandle_t d)
{
    dir_info* di = (dir_info*)d;
    
    if (!di)
    {
        return false;
    }

    BOOL bRes = FindClose(di->m_handle);
    delete di;
    return bRes ? true : false;
}



xf_file_t xf_open(const std::wstring& filename,
                  unsigned int disposition,
                  unsigned int access_flags,
                  unsigned int share_flags)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareFlags = 0;
    DWORD dwDisposition;
    if (access_flags & xfRead)
        dwDesiredAccess |= GENERIC_READ;
    if (access_flags & xfWrite)
        dwDesiredAccess |= GENERIC_WRITE;
    if (share_flags & xfShareRead)
        dwShareFlags |= FILE_SHARE_READ;
    if (share_flags & xfShareWrite)
        dwShareFlags |= FILE_SHARE_WRITE;
    switch (disposition)
    {
        case xfOpen : dwDisposition = OPEN_EXISTING; break;
        case xfOpenCreateIfNotExist: dwDisposition = OPEN_ALWAYS; break;
        case xfCreate: dwDisposition = CREATE_ALWAYS; break;
        default: return NULL;
    }
    
    int opt = 0;
    //int opt = FILE_FLAG_RANDOM_ACCESS;
    //int opt = FILE_FLAG_SEQUENTIAL_SCAN;
#ifdef _UNICODE
    HANDLE h = CreateFile(filename.c_str(), dwDesiredAccess, dwShareFlags,
                NULL, dwDisposition, FILE_ATTRIBUTE_NORMAL | opt, NULL);
#else
    HANDLE h = CreateFile(kl::tstr(filename), dwDesiredAccess, dwShareFlags,
                NULL, dwDisposition, FILE_ATTRIBUTE_NORMAL | opt, NULL);
#endif

    #ifdef _DEBUG
    if (h == INVALID_HANDLE_VALUE)
    {
        std::wstring last_error_msg = xf_get_win32_error_msg();
        int i = 5;
    }
    #endif

    return (h != INVALID_HANDLE_VALUE ? h : NULL);
}


bool xf_flush(xf_file_t fileh)
{
    return (FlushFileBuffers(fileh) ? true : false);
}

bool xf_close(xf_file_t fileh)
{
    return (CloseHandle(fileh) ? true : false);
}

bool xf_seek(xf_file_t fileh, xf_off_t seek_pos, unsigned int _method)
{
    LONG lowpart, hipart;
    DWORD result;
    unsigned method;
    
    switch (_method) 
    {
        case xfSeekCur: method = FILE_CURRENT; break;
        case xfSeekEnd: method = FILE_END; break;
        case xfSeekSet: method = FILE_BEGIN; break;
        default:
            return false;
    }
    
    #ifdef _MSC_VER
    lowpart = (LONG)(seek_pos & (xf_off_t)0x00000000ffffffff);
    hipart = (LONG)((seek_pos & (xf_off_t)0xffffffff00000000) >> 32);
    #else
    lowpart = (LONG)(seek_pos & (xf_off_t)0x00000000ffffffffLL);
    hipart = (LONG)((seek_pos & (xf_off_t)0xffffffff00000000LL) >> 32);
    #endif
    result = SetFilePointer(fileh, lowpart, &hipart, method);
    return (result == 0xFFFFFFFF && GetLastError() != NO_ERROR ? false : true);
} 

int xf_read(xf_file_t fileh,
            void* buffer,
            unsigned int read_size,
            unsigned int read_count)
{
    DWORD read = 0;
    if (read_size == 0)
        return 0;
    ReadFile(fileh, buffer, read_size*read_count, &read, NULL);
    return (read/read_size);
}

int xf_write(xf_file_t fileh,
             const void* buffer,
             unsigned int write_size,
             unsigned int write_count)
{
    DWORD written = 0;
    if (write_size == 0)
        return 0;
    WriteFile(fileh, buffer, write_size*write_count, &written, NULL);
    return (written/write_size);
}

bool xf_lock(xf_file_t fileh, xf_off_t offset, xf_off_t len)
{
    DWORD off_low, off_hi;
    DWORD len_low, len_hi;
    #ifdef _MSC_VER
    off_low = (DWORD)(offset & (xf_off_t)0x00000000ffffffff);
    off_hi = (DWORD)((offset & (xf_off_t)0xffffffff00000000) >> 32);
    len_low = (DWORD)(len & (xf_off_t)0x00000000ffffffff);
    len_hi = (DWORD)((len & (xf_off_t)0xffffffff00000000) >> 32);
    #else
    off_low = (DWORD)(offset & (xf_off_t)0x00000000ffffffffLL);
    off_hi = (DWORD)((offset & (xf_off_t)0xffffffff00000000LL) >> 32);
    len_low = (DWORD)(len & (xf_off_t)0x00000000ffffffffLL);
    len_hi = (DWORD)((len & (xf_off_t)0xffffffff00000000LL) >> 32);
    #endif

    return LockFile(fileh, off_low, off_hi, len_low, len_hi) ? true : false;
}

bool xf_unlock(xf_file_t fileh, xf_off_t offset, xf_off_t len)
{
    DWORD off_low, off_hi;
    DWORD len_low, len_hi;
    #ifdef _MSC_VER
    off_low = (DWORD)(offset & (xf_off_t)0x00000000ffffffff);
    off_hi = (DWORD)((offset & (xf_off_t)0xffffffff00000000) >> 32);
    len_low = (DWORD)(len & (xf_off_t)0x00000000ffffffff);
    len_hi = (DWORD)((len & (xf_off_t)0xffffffff00000000) >> 32);
    #else
    off_low = (DWORD)(offset & (xf_off_t)0x00000000ffffffffLL);
    off_hi = (DWORD)((offset & (xf_off_t)0xffffffff00000000LL) >> 32);
    len_low = (DWORD)(len & (xf_off_t)0x00000000ffffffffLL);
    len_hi = (DWORD)((len & (xf_off_t)0xffffffff00000000LL) >> 32);
    #endif

    return UnlockFile(fileh, off_low, off_hi, len_low, len_hi) ? true : false;
}

bool xf_trylock(xf_file_t fileh, xf_off_t offset, xf_off_t len, int milliseconds)
{
    while (1)
    {
        if (xf_lock(fileh, offset, len))
            return true;

        Sleep(10);
        milliseconds -= 10;
        if (milliseconds <= 0)
        {
            break;
        }
    }

    return false;
}


bool xf_truncate(xf_file_t fileh)
{
    SetFilePointer(fileh, 0, NULL, FILE_BEGIN);
    return (SetEndOfFile(fileh) != 0 ? true : false);
}


xf_off_t xf_get_file_pos(xf_file_t fileh)
{
    DWORD fsl = 0;
    LONG fsh = 0;
    fsl = SetFilePointer(fileh, 0, &fsh, FILE_CURRENT);
    return ((((xf_off_t)fsh) << 32) | (fsl));
}


// xf_filetime_t is number of 100-nanosecond intervals since 1/1/1601
xf_filetime_t xf_get_file_modify_time(const std::wstring& filename)
{
    HANDLE h;
    WIN32_FIND_DATA fd;
#ifdef _UNICODE
    h = FindFirstFile(filename.c_str(), &fd);
#else
    h = FindFirstFile(kl::tstr(filename), &fd);
#endif
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    FindClose(h);
    
    xf_filetime_t ft = fd.ftLastWriteTime.dwHighDateTime;
    ft <<= 32;
    ft |= fd.ftLastWriteTime.dwLowDateTime;
    
    return ft;
}


// xf_filetime_t is number of 100-nanosecond intervals since 1/1/1601
/*
xf_filetime_t xf_get_directory_create_time(const std::wstring& dir)
{
    HANDLE h;
    FILETIME ft;
#ifdef _UNICODE
    h = CreateFile(dir.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_DIRECTORY, NULL);
#else
    h = CreateFile(kl::tstr(dir), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_DIRECTORY, NULL);
#endif
    if(h == INVALID_HANDLE_VALUE)
        return 0;
    
    BOOL res = GetFileTime(h, &ft, NULL, NULL);
    if (!res)
        return 0;
    CloseHandle(h);
    
    xf_filetime_t ft = ft.dwHighDateTime;
    ft <<= 32;
    ft |= fd.dwLowDateTime;
    
    return ft;
}
*/


void xf_filetime_to_systemtime(xf_filetime_t ft, xf_timestruct_t* out)
{
    // because the FileTimeToSystemTime can be off
    // if DST is not taken into consideration, use
    // the crt localtime to get the
    xf_filetime_t ft2 = ft;
    ft2 -= 116444736000000000;
    ft2 /= 10000000;
    
    time_t t = (time_t)ft2;
    struct tm* pts = localtime(&t);
    if (!pts)
    {
        // error occurred
        return; // false;
    }
    
    struct tm ts = *pts;
    
    out->year = ts.tm_year+1900;
    out->month = ts.tm_mon;
    out->day = ts.tm_mday;
    out->hour = ts.tm_hour;
    out->minute = ts.tm_min;
    out->second = ts.tm_sec;
    out->milliseconds = 0;
    out->dow = ts.tm_wday;

    // use the win32 to get file's high-resolution milliseconds
    FILETIME f;
    SYSTEMTIME st;
    f.dwHighDateTime = (DWORD)((ft >> 32) & 0xffffffff);
    f.dwLowDateTime = (DWORD)(ft & 0xffffffff);
    FileTimeToSystemTime(&f, &st);
    
    out->milliseconds = st.wMilliseconds;
}


/*
xf_off_t xf_get_file_size(xf_file_t fileh)
{
    xf_off_t file_size;
    DWORD fsh, fsl;
    fsl = GetFileSize(fileh, &fsh);
    file_size = ((((xf_off_t)fsh) << 32) | (fsl));
    return file_size;
}
*/

bool xf_get_file_exist(const std::wstring& file_path)
{
    HANDLE h;
    WIN32_FIND_DATA fd;
#ifdef _UNICODE
    h = FindFirstFile(file_path.c_str(), &fd);
#else
    h = FindFirstFile(kl::tstr(file_path), &fd);
#endif
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

bool xf_get_directory_exist(const std::wstring& dir_path)
{
    std::wstring path = dir_path;
    if (path.empty())
        return false;

    if (path[path.length()-1] != L'\\')
        path += L"\\*.*";
         else
        path += L"*.*";

    HANDLE h;
    WIN32_FIND_DATA fd;
#ifdef _UNICODE
    h = FindFirstFile(path.c_str(), &fd);
#else
    h = FindFirstFile(kl::tstr(path), &fd);
#endif

    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}


bool xf_is_valid_file_path(const std::wstring& file_path)
{
    // this function applies specifically to paths which are structured
    // like "c:\directory\file.ext", and does not encompass network
    // paths such as "\\server\directory"
    
    if (file_path.length() >= 255)
        return false;
    
    int dir_separator_idx = file_path.find_last_of(L"\\");
    std::wstring dir_path  = file_path.substr(0, dir_separator_idx);
    std::wstring file_name = file_path.substr(dir_separator_idx+1);
    
    // check the directory path portion of
    // the file name to make sure it is valid
    if (!xf_is_valid_directory_path(dir_path))
        return false;
    
    if (file_name.length() == 0)
        return false;
    
    const wchar_t* p = file_name.c_str();
    while (*p)
    {
        // go to http://msdn.microsoft.com/en-us/library/aa365247.aspx
        // for more documentation on invalid filename and directory
        // characters in windows
        
        if (*p > 0 && *p <= 31)
            return false;
        
        // invalid file characters
        if (*p == L'<'  ||
            *p == L'>'  ||
            *p == L':'  ||
            *p == L'"'  ||
            *p == L'/'  ||
            *p == L'\\' ||
            *p == L'|'  ||
            *p == L'?'  ||
            *p == L'*')
        {
            return false;
        }
        
        ++p;
    }
    
    return true;
}

bool xf_is_valid_directory_path(const std::wstring& dir_path)
{
    // this function applies specifically to paths which are structured
    // like "c:\directory\directory", and does not encompass network
    // paths such as "\\server\directory"
    
    if (dir_path.length() >= 255)
        return false;
    
    int vol_separator_idx = dir_path.find_first_of(L":");
    std::wstring vol  = dir_path.substr(0, vol_separator_idx);
    std::wstring dirs = dir_path.substr(vol_separator_idx+1);
    
    // volume mount must be only one character in windows
    if (vol.length() != 1)
        return false;
    
    wchar_t last_p = ' ';
    const wchar_t* p = dirs.c_str();
    while (*p)
    {
        // go to http://msdn.microsoft.com/en-us/library/aa365247.aspx
        // for more documentation on invalid filename and directory
        // characters in windows
        
        if (*p > 0 && *p <= 31)
            return false;
        
        // can't have embedded "\\" strings in directory paths
        if (*p == L'\\' && last_p == L'\\')
            return false;
        
        // invalid directory characters
        if (*p == L'<' ||
            *p == L'>' ||
            *p == L':' ||
            *p == L'"' ||
            *p == L'/' ||
            *p == L'|' ||
            *p == L'?' ||
            *p == L'*')
        {
            return false;
        }
        
        last_p = *p;
        ++p;
    }
    
    return true;
}

bool xf_mkdir(const std::wstring& dir)
{
#ifdef _UNICODE
    return (CreateDirectory(dir.c_str(), NULL) ? true : false);
#else
    return (CreateDirectory(kl::tstr(dir), NULL) ? true : false);
#endif
}

bool xf_rmdir(const std::wstring& dir)
{
#ifdef _UNICODE
    return RemoveDirectory(dir.c_str()) ? true : false;
#else
    return RemoveDirectory(kl::tstr(dir)) ? true : false;
#endif
}


static std::wstring makeFilePath(const std::wstring& dir, const std::wstring& fname)
{
    std::wstring res = dir;
    if (res.length() == 0 || res[res.length()-1] != '\\')
        res += L"\\";
    res += fname;
    return res;
}

bool xf_rmtree(const std::wstring& path)
{
    std::vector<std::wstring> to_remove;

    xf_dirhandle_t h = xf_opendir(path);
    xf_direntry_t info;
    while (xf_readdir(h, &info))
    {
        // skip . and ..
        if (info.m_name == L"." || info.m_name == L"..")
            continue;
        
        if (info.m_type == xfFileTypeNormal)
            xf_remove(makeFilePath(path, info.m_name));
        if (info.m_type == xfFileTypeDirectory)
            to_remove.push_back(makeFilePath(path, info.m_name));
    }
    xf_closedir(h);

    std::vector<std::wstring>::iterator it;
    for (it = to_remove.begin(); it != to_remove.end(); ++it)
        xf_rmtree(*it);

    xf_rmdir(path);

    return true;
}


bool xf_move(const std::wstring& current_path, const std::wstring& new_path)
{
#ifdef _UNICODE
    return MoveFile(current_path.c_str(), new_path.c_str()) ? true : false;
#else
    return MoveFile(kl::tstr(current_path), kl::tstr(new_path)) ? true : false;
#endif
}

bool xf_remove(const std::wstring& filename)
{
#ifdef _UNICODE
    return DeleteFile(filename.c_str()) ? true : false;
#else
    return DeleteFile(kl::tstr(filename)) ? true : false;
#endif
}

xf_off_t xf_get_file_size(const std::wstring& filename)
{
    HANDLE h;
    WIN32_FIND_DATA fd;
#ifdef _UNICODE
    h = FindFirstFile(filename.c_str(), &fd);
#else
    h = FindFirstFile(kl::tstr(filename), &fd);
#endif
    if (h == INVALID_HANDLE_VALUE)
        return -1;
    FindClose(h);

    xf_off_t file_size = fd.nFileSizeHigh;
    file_size <<= 32;
    file_size += fd.nFileSizeLow;

    return file_size;
}

xf_off_t xf_get_free_disk_space(const std::wstring& _path)
{
    if (_path.length() == 0)
        return 0;

    std::wstring path;

    if (_path.substr(0,2) == L"\\\\")
    {
        std::wstring unc;

        // -- UNC path name --
        path = _path.substr(2);

        int pos = path.find_first_of(L"\\");
        if (pos == -1)
            return 0;

        unc = L"\\\\";
        unc += path.substr(0, pos);
        unc += L"\\";

        path = kl::afterFirst(path, L'\\');
        pos = path.find_first_of(L"\\");
        
        if (pos == -1)
        {
            unc += path;
        }
         else
        {
            unc += path.substr(0, pos);
        }

        path = unc;
    }
     else
    {
        int pos = _path.find_first_of(L"\\");
        if (pos == -1)
        {
            path = _path;
        }
         else
        {
            path = _path.substr(0, pos);
        }
    }

#ifdef _UNICODE
    typedef BOOL (WINAPI *GetDiskFreeSpaceExFunc)(LPCWSTR,
                                                PULARGE_INTEGER,
                                                PULARGE_INTEGER,
                                                PULARGE_INTEGER);
#else
    typedef BOOL (WINAPI *GetDiskFreeSpaceExFunc)(LPCSTR,
                                                PULARGE_INTEGER,
                                                PULARGE_INTEGER,
                                                PULARGE_INTEGER);
#endif

    static GetDiskFreeSpaceExFunc pGetDiskFreeSpaceEx = NULL;

    if (!pGetDiskFreeSpaceEx)
    {
#ifdef _UNICODE
        pGetDiskFreeSpaceEx = (GetDiskFreeSpaceExFunc)::GetProcAddress(
                            ::GetModuleHandleW(L"kernel32.dll"),
                            "GetDiskFreeSpaceExW");
#else
        pGetDiskFreeSpaceEx = (GetDiskFreeSpaceExFunc)::GetProcAddress(
                            ::GetModuleHandleA("kernel32.dll"),
                            "GetDiskFreeSpaceExA");
#endif
    }

    if (pGetDiskFreeSpaceEx)
    {
        ULARGE_INTEGER bytesFree, bytesTotal;

#ifdef _UNICODE
        if ( !pGetDiskFreeSpaceEx(path.c_str(),
                                  &bytesFree,
                                  &bytesTotal,
                                  NULL) )
        {
            return 0;
        }
#else
        if ( !pGetDiskFreeSpaceEx(kl::tstr(path),
                                  &bytesFree,
                                  &bytesTotal,
                                  NULL) )
        {
            return 0;
        }
#endif

        return bytesFree.QuadPart;
    }
     else
    {
        DWORD lSectorsPerCluster, lBytesPerSector,
              lNumberOfFreeClusters, lTotalNumberOfClusters;

        if ( !::GetDiskFreeSpace(kl::tstr(path),
                                 &lSectorsPerCluster,
                                 &lBytesPerSector,
                                 &lNumberOfFreeClusters,
                                 &lTotalNumberOfClusters) )
        {
            return 0;
        }

        xf_off_t retval = lSectorsPerCluster;
        retval *= lBytesPerSector;
        retval *= lNumberOfFreeClusters;

        return retval;
    }
}


std::wstring xf_get_temp_path()
{
    // -- find a suitable tempfile path default --
    
    #ifdef _UNICODE
        wchar_t buf[512];
        ::GetTempPathW(512, buf);
    #else
        char buf[512];
        ::GetTempPathA(512, buf);
    #endif
    
    std::wstring res = (wchar_t*)kl::tstr(buf);
    
    if (res.empty() || res[res.length()-1] != L'\\')
    {
        res+= L"\\";
    }

    return res;
}

std::wstring xf_get_temp_filename(const std::wstring& prefix, const std::wstring& extension)
{
    std::wstring result = xf_get_temp_path();
    result += prefix;
    
    static int cnt = 0;
    
    if (cnt == 0)
    {
        cnt = rand();
    }
    
    cnt++;
        
    wchar_t buf[255];
    swprintf(buf, L"%04x%04x%04x%04x",
              ((int)time(NULL) & 0xffff),
              ((int)clock() & 0xffff),
              (rand() & 0xffff),
              (cnt % 0xffff));
              
    result += buf;
    result += L".";
    
    if (extension.length() > 0)
    {
        result += extension;
    }
     else
    {
        result += L"tmp";
    }

    return result;
}

std::wstring xf_get_network_path(const std::wstring& filename)
{
    TCHAR buf[1024];
    UNIVERSAL_NAME_INFO* info;
    DWORD info_size = sizeof(TCHAR) * 1024;
    DWORD err;
    
    info = (UNIVERSAL_NAME_INFO*)buf;
    
#ifdef _UNICODE
    err = WNetGetUniversalNameW((LPCWSTR)filename.c_str(),
                                UNIVERSAL_NAME_INFO_LEVEL,
                                (LPVOID)info,
                                &info_size);
    if (err != NO_ERROR)
        return filename;
        
    return info->lpUniversalName;
#else
    err = WNetGetUniversalNameA(kl::tstr(filename),
                                UNIVERSAL_NAME_INFO_LEVEL,
                                (LPVOID)info,
                                &info_size);
    if (err != NO_ERROR)
        return filename;
        
    return kl::towstring(info->lpUniversalName);
#endif
  
}

