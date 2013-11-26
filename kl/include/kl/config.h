/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2013-11-26
 *
 */


#ifndef __KL_CONFIG_H
#define __KL_CONFIG_H

#include <string>
#include <vector>


namespace kl
{


class ConfigImplBase;
class Config
{
public:

    Config();

    void setPath(const std::wstring& path);

    bool exists(const std::wstring& path);

    bool write(const std::wstring& path, const std::wstring& value);
    bool write(const std::wstring& path, long value);
    bool write(const std::wstring& path, bool value) { return write(path, (long)(value ? 1 : 0)); }

    bool read(const std::wstring& path, std::wstring& value, const std::wstring& def = L"");
    bool read(const std::wstring& path, long* value, long def);
    bool read(const std::wstring& path, bool* value, bool def = false)
    {
        long l;
        bool res = read(path, &l, def ? 1 : 0);
        *value = (l == 0 ? false : true);
        return res;
    }

    std::vector<std::wstring> getGroups(const std::wstring& path = L"");
    bool deleteGroup(const std::wstring& path = L"");

private:

    ConfigImplBase* m_impl;
};





};



#endif

