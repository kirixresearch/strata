/*!
 *
 * Copyright (c) 2006-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-30
 *
 */


#include <cstdio>
#include <vector>
#include <sys/stat.h>


#ifndef _MSC_VER
#ifdef __APPLE__
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif


#ifdef _MSC_VER
#include <direct.h>     // for mkdir and rmdir
#endif


#include "kl/file.h"
#include "kl/string.h"


#ifdef _MSC_VER
typedef __int64 xf_off_t;
typedef __int64 xf_filetime_t;
#else
typedef long long xf_off_t;
typedef long long xf_filetime_t;
#endif



struct dir_info
{
    DIR* m_handle;
    std::wstring m_path;
};



xf_dirhandle_t xf_opendir(const std::wstring& dir)
{
    std::string s_dir = kl::tostring(dir);

    dir_info* info = new dir_info;
    info->m_path = dir;
    info->m_handle = opendir(s_dir.c_str());

    if (info->m_handle == NULL)
    {
        delete info;
        return NULL;
    }

    return info;
}

bool xf_readdir(xf_dirhandle_t d, xf_direntry_t* entry)
{
    dir_info* info;
    struct dirent dirent;
    struct dirent* direntp;

    if (!d)
    {
        return false;
    }

    info = (dir_info*)d;


    readdir_r(info->m_handle, &dirent, &direntp);

    if (direntp == NULL)
    {
        return false;
    }

    entry->m_name = kl::towstring(direntp->d_name);

    if (direntp->d_type == DT_DIR)
    {
        entry->m_type = xfFileTypeDirectory;
    }
     else if (direntp->d_type == DT_REG)
    {
        entry->m_type = xfFileTypeNormal;
    }
     else if (direntp->d_type == DT_UNKNOWN)
    {
        std::wstring fullname;
        fullname = info->m_path;
        fullname += L"/";
        fullname += entry->m_name;

        std::string s_fullname = kl::tostring(fullname);

        struct stat statinfo;

        if (stat(s_fullname.c_str(), &statinfo) == 0)
        {
            entry->m_type = xfFileTypeNormal;

            if (S_ISDIR(statinfo.st_mode))
            {
                entry->m_type = xfFileTypeDirectory;
            }
        }
         else
        {
            entry->m_type = xfFileTypeNormal;
        }
    }
     else
    {
        entry->m_type = xfFileTypeNormal;
    }

    return true;
}

bool xf_closedir(xf_dirhandle_t d)
{
    if (!d)
    {
        return false;
    }

    dir_info* info = (dir_info*)d;
    DIR* handle = info->m_handle;
    delete info;

    return (closedir(handle) == 0 ? true : false);
}


    
xf_file_t xf_open(const std::wstring& filename,
                  unsigned int disposition,
                  unsigned int access_flags,
                  unsigned int share_flags)
{
    char access[64];
    access[0] = 0;

    std::string s_filename = kl::tostring(filename);
    
    if (access_flags & xfRead)
        strcat(access, "rb");
    if (access_flags & xfWrite)
        strcat(access, "r+b");

    if (disposition == xfOpen)
    {
        return fopen(s_filename.c_str(), access);
    }
     else if (disposition == xfOpenCreateIfNotExist)
    {
        bool exist = true;
        struct stat stat_struct;
        if (stat(s_filename.c_str(), &stat_struct) != 0)
            exist = false;
        if (exist && (stat_struct.st_mode & S_IFDIR))
            return 0;
        if (exist)
        {
            return fopen(s_filename.c_str(), access);
        }
         else
        {
            return fopen(s_filename.c_str(), "w+b");
        }
    }
     else if (disposition == xfCreate)
    {
        return fopen(s_filename.c_str(), "w+b");
    }

    return 0;
}

bool xf_close(xf_file_t fileh)
{
    return (fclose((FILE*)fileh) == 0 ? true : false);
}

bool xf_seek(xf_file_t fileh,
             xf_off_t seek_pos,
             unsigned int _method)
{
    int method;
    
    switch (_method) 
    {
        case xfSeekCur: method = SEEK_CUR; break;
        case xfSeekEnd: method = SEEK_END; break;
        case xfSeekSet: method = SEEK_SET; break;
        default:
            return false;
    }
    
    #ifdef _MSC_VER
        return (fseek((FILE*)fileh, (long)seek_pos, method) == 0) ? true : false;
    #else
        return (fseeko((FILE*)fileh, seek_pos, method) == 0) ? true : false;
    #endif
} 

int xf_read(xf_file_t fileh,
            void* buffer,
            unsigned int read_size,
            unsigned int read_count)
{
    return fread(buffer, read_size, read_count, (FILE*)fileh);
}

int xf_write(xf_file_t fileh,
             const void* buffer,
             unsigned int write_size,
             unsigned int write_count)
{
    return fwrite(buffer, write_size, write_count, (FILE*)fileh);
}

bool xf_lock(xf_file_t fileh, xf_off_t offset, xf_off_t len)
{
    return true;
}

bool xf_unlock(xf_file_t fileh, xf_off_t offset, xf_off_t len)
{
    return true;
}

bool xf_trylock(xf_file_t fileh, xf_off_t offset, xf_off_t len, int milliseconds)
{
    return true;
}

xf_off_t xf_get_file_pos(xf_file_t fileh)
{
    fpos_t pos;
    fgetpos((FILE*)fileh, &pos);
    #if defined  _MSC_VER || defined __APPLE__
        return pos;
    #else
        return pos.__pos;
    #endif
}

bool xf_get_file_exist(const std::wstring& file_path)
{
    struct stat stat_struct;
    if (stat(kl::tostring(file_path).c_str(), &stat_struct) != 0)
        return false;
    if (stat_struct.st_mode & S_IFDIR)
        return false;
    return true;
}

bool xf_get_directory_exist(const std::wstring& dir_path)
{
    struct stat stat_struct;
    if (stat(kl::tostring(dir_path).c_str(), &stat_struct) != 0)
        return false;
    if (stat_struct.st_mode & S_IFDIR)
        return true;
    return false;
}

bool xf_is_valid_file_path(const std::wstring& file_path)
{
    return true;
}

bool xf_is_valid_directory_path(const std::wstring& dir_path)
{
    return true;
}

bool xf_mkdir(const std::wstring& dir)
{
#ifdef _MSC_VER
    return (mkdir(kl::tostring(dir).c_str()) == 0) ? true : false;
#else
    return (mkdir(kl::tostring(dir).c_str(), 0777) == 0) ? true : false;
#endif
}

bool xf_rmdir(const std::wstring& dir)
{
    return (rmdir(kl::tostring(dir).c_str()) == 0) ? true : false;
}


// identical to win32 version (generic function)

static std::wstring makeFilePath(const std::wstring& dir, const std::wstring& fname)
{
    std::wstring res = dir;
    if (res.length() == 0 || res[res.length()-1] != '/')
        res += L"/";
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
    return (rename(kl::tostring(current_path).c_str(),
                   kl::tostring(new_path).c_str()) == 0) ? true : false;
}

bool xf_remove(const std::wstring& filename)
{
    return (remove(kl::tostring(filename).c_str()) == 0) ? true : false;
}

xf_off_t xf_get_file_size(const std::wstring& filename)
{
    struct stat s;
    if (0 != stat(kl::tostring(filename).c_str(), &s))
        return -1;
    return s.st_size;
}

xf_filetime_t xf_get_file_modify_time(const std::wstring& filename)
{
    struct stat s;
    if (0 != stat(kl::tostring(filename).c_str(), &s))
        return 0;
    
    xf_filetime_t ft = s.st_mtime;
    ft *= 10000000;
    return s.st_mtime;
}

xf_filetime_t xf_get_directory_create_time(const std::wstring& dir)
{
    return 0;
}


void xf_filetime_to_systemtime(xf_filetime_t ft, xf_timestruct_t* out)
{
    struct tm ts;
    time_t t;
    
    ft -= 116444736000000000LL;
    ft /= 10000000;
    
    t = (time_t)ft;
    localtime_r(&t, &ts);
    
    out->year = ts.tm_year+1900;
    out->month = ts.tm_mon;
    out->day = ts.tm_mday;
    out->hour = ts.tm_hour;
    out->minute = ts.tm_min;
    out->second = ts.tm_sec;
    out->milliseconds = 0;
    out->dow = ts.tm_wday;
    
/*
    // I'm not entirely sure this is correct or necessary
    // the CRT function should work well by itself, but I'll
    // leave the code here in case it becomes necessary
    // BIW 3/3/07

    bool cur_time_is_dst;
    struct tm ts;
    time_t t;
    
    // first determine is current time is DST
    time(&t);
    thread_safe_localtime(&t, &ts);
    cur_time_is_dst = ts.tm_isdst ? true : false;



    ft -= 116444736000000000;
    ft /= 10000000;
    

    
    t = (time_t)ft;
    thread_safe_localtime(&t, &ts);
    
    // if day light savings time differs, compensate
    if (cur_time_is_dst)
    {
        if (!ts.tm_isdst)
        {
            t += 3600;
            thread_safe_localtime(&t, &ts);
        }
    }
     else
    {
        if (ts.tm_isdst)
        {
            t -= 3600;
            thread_safe_localtime(&t, &ts);
        }
    }
    
    
    out->year = ts.tm_year+1900;
    out->month = ts.tm_mon;
    out->day = ts.tm_mday;
    out->hour = ts.tm_hour;
    out->minute = ts.tm_min;
    out->second = ts.tm_sec;
    out->milliseconds = 0;
    out->dow = ts.tm_wday;
*/
}



xf_off_t xf_get_free_disk_space(const std::wstring& path)
{
    struct statfs s;
    statfs(kl::tostring(path).c_str(), &s);
    xf_off_t retval = s.f_bfree;
    retval *= 512;
    return retval;
}

std::wstring xf_get_temp_path()
{
    return L"/tmp/";
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
    swprintf(buf, 255, L"%04x%04x%04x%04x",
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
    return filename;
}

