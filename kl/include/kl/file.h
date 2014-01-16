/*!
 *
 * Copyright (c) 2006-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2006-03-30
 *
 */


#ifndef __KL_FILE_H
#define __KL_FILE_H


#include <string>



#ifdef _MSC_VER
const char xf_path_separator_char = '\\';
const wchar_t xf_path_separator_wchar = L'\\';
#else
const char xf_path_separator_char = '/';
const wchar_t xf_path_separator_wchar = L'\\';
#endif


// -- directory API --

const int xfFileTypeNormal = 1;
const int xfFileTypeDirectory = 2;

struct xf_direntry_t
{
    std::wstring m_name;
    int m_type;
};

typedef void* xf_dirhandle_t;

xf_dirhandle_t xf_opendir(const std::wstring& dir);
bool xf_readdir(xf_dirhandle_t d, xf_direntry_t* entry);
bool xf_closedir(xf_dirhandle_t d);



// -- file API portability --

enum
{
    xfOpen = 0x01,
    xfOpenCreateIfNotExist = 0x02,
    xfCreate = 0x03
};

enum
{
    xfRead = 0x01,
    xfWrite = 0x02,
    xfText = 0x04,
    xfReadWrite = (0x01 | 0x02)
};

enum
{
    xfShareNone = 0x00,
    xfShareRead = 0x01,
    xfShareWrite = 0x02,
    xfShareReadWrite = (0x01 | 0x02)
};


enum
{
    xfSeekCur = 0x01,
    xfSeekEnd = 0x02,
    xfSeekSet = 0x03
};

#ifdef _MSC_VER
typedef __int64 xf_off_t;
typedef __int64 xf_filetime_t;
#else
typedef long long xf_off_t;
typedef long long xf_filetime_t;
#endif

typedef void* xf_file_t;


struct xf_timestruct_t
{
    int year;
    int month;
    int day;
    int dow;
    int hour;
    int minute;
    int second;
    int milliseconds;
};


xf_file_t xf_open(const std::wstring& filename,
                  unsigned int disposition,
                  unsigned int access_flags,
                  unsigned int share_flags);
bool xf_flush(xf_file_t fileh);
bool xf_close(xf_file_t fileh);
bool xf_seek(xf_file_t fileh, xf_off_t seek_pos, unsigned int method);
int xf_read(xf_file_t fileh, void* buffer, unsigned int read_size, unsigned int read_count);
int xf_write(xf_file_t fileh, const void* buffer, unsigned int write_size, unsigned int write_count);
bool xf_lock(xf_file_t fileh, xf_off_t offset, xf_off_t len);
bool xf_unlock(xf_file_t fileh, xf_off_t offset, xf_off_t len);
bool xf_trylock(xf_file_t fileh, xf_off_t offset, xf_off_t len, int milliseconds);
bool xf_truncate(xf_file_t fileh);
xf_off_t xf_get_file_pos(xf_file_t fileh);
xf_filetime_t xf_get_file_modify_time(const std::wstring& filename);
//xf_filetime_t xf_get_directory_create_time(const std::wstring& dir);
void xf_filetime_to_systemtime(xf_filetime_t ft, xf_timestruct_t* out);
//inline xf_off_t xf_get_file_size(xf_file_t fileh);
bool xf_get_file_exist(const std::wstring& file_path);
bool xf_get_directory_exist(const std::wstring& dir_path);
std::wstring xf_get_file_directory(const std::wstring& filename);
bool xf_mkdir(const std::wstring& dir);
bool xf_rmdir(const std::wstring& dir);
bool xf_rmtree(const std::wstring& dir);
bool xf_move(const std::wstring& current_path, const std::wstring& new_path);
bool xf_remove(const std::wstring& filename);
bool xf_is_valid_file_path(const std::wstring& file_path);
bool xf_is_valid_directory_path(const std::wstring& dir_path);
xf_off_t xf_get_file_size(const std::wstring& filename);
xf_off_t xf_get_free_disk_space(const std::wstring& _path);
std::wstring xf_get_temp_path();
std::wstring xf_get_temp_filename(const std::wstring& prefix, const std::wstring& extension);
std::wstring xf_get_network_path(const std::wstring& filename);
std::wstring xf_get_file_contents(const std::wstring& filename, bool* success = NULL);
bool xf_put_file_contents(const std::wstring& filename, const std::wstring& contents);



namespace kl
{

class exclusive_file
{
public:

    exclusive_file(const std::wstring& path, int timeout = 10 /* 10 seconds */);
    ~exclusive_file();
    bool isOk() const { return m_f ? true : false; }
    std::wstring getContents(bool* success = NULL);
    bool putContents(const std::wstring& contents);

private:

    std::wstring m_path;
    int m_timeout;
    xf_file_t m_f;
    time_t m_time;
    bool m_existed;
};

};

#endif      // __KL_FILE_H

