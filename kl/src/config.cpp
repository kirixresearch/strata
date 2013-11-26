/*!
 *
 * Copyright (c) 2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2013-11-26
 *
 */


#define _CRT_SECURE_NO_WARNINGS
#include <kl/portable.h>
#include <kl/config.h>


namespace kl
{



class ConfigImplBase
{
public:

    ConfigImplBase() { }
    virtual ~ConfigImplBase() { }

    virtual void setPath(const std::wstring& path) = 0;
    virtual bool exists(const std::wstring& path) = 0;

    virtual bool write(const std::wstring& path, const std::wstring& value) = 0;
    virtual bool write(const std::wstring& path, long value) = 0;

    virtual bool read(const std::wstring& path, std::wstring& value, const std::wstring& def = L"") = 0;
    virtual bool read(const std::wstring& path, long* value, long def) = 0;

    virtual std::vector<std::wstring> getGroups(const std::wstring& path = L"") = 0;
    virtual bool deleteGroup(const std::wstring& path = L"") = 0;

protected:

    std::wstring m_path;
};




#ifdef WIN32


class ConfigWinRegImpl : public ConfigImplBase
{
public:

    ConfigWinRegImpl()
    {
    }

    void setPath(const std::wstring& path)
    {
        m_path = path;
    }

    bool exists(const std::wstring& path);

    bool write(const std::wstring& path, const std::wstring& value);
    bool write(const std::wstring& path, long value);

    bool read(const std::wstring& path, std::wstring& value, const std::wstring& def);
    bool read(const std::wstring& path, long* value, long def);


    std::vector<std::wstring> getGroups(const std::wstring& path)
    {
    }

    bool deleteGroup(const std::wstring& path)
    {
    }

private:

    std::wstring m_path;

};


#endif





Config::Config()
{

}

void Config::setPath(const std::wstring& path)
{
    m_impl->setPath(path);
}

bool Config::exists(const std::wstring& path)
{
    return m_impl->exists(path);
}

bool Config::write(const std::wstring& path, const std::wstring& value)
{
    return m_impl->write(path, value);
}

bool Config::write(const std::wstring& path, long value)
{
    return m_impl->write(path, value);
}

bool Config::read(const std::wstring& path, std::wstring& value, const std::wstring& def)
{
    return m_impl->read(path, value, def);
}

bool Config::read(const std::wstring& path, long* value, long def)
{
    return m_impl->read(path, value, def);
}

std::vector<std::wstring> Config::getGroups(const std::wstring& path)
{
    return m_impl->getGroups(path);
}

bool Config::deleteGroup(const std::wstring& path)
{
    return m_impl->deleteGroup(path);
}



}; // namespace kl

