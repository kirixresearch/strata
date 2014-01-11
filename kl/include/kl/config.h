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


class config_impl_base;
class config
{
public:

    config();
    ~config();

    bool init(const std::wstring& organization, const std::wstring& product);

    void setPath(const std::wstring& path);

    bool exists(const std::wstring& path);

    bool write(const std::wstring& path, const std::wstring& value);
    bool write(const std::wstring& path, long value);

    bool read(const std::wstring& path, std::wstring* value, const std::wstring& def = L"");
    bool read(const std::wstring& path, long* value, long def);

    std::wstring read(const std::wstring& path, const std::wstring& def = L"") {  std::wstring res; read(path, &res, def); return res; }

    std::vector<std::wstring> getGroups(const std::wstring& path = L"");
    bool deleteGroup(const std::wstring& path = L"");

    std::wstring operator[](const std::wstring& path) { return read(path); }

private:

    config_impl_base* m_impl;
};





};



#endif

