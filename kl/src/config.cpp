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
#include <kl/string.h>
#include <kl/config.h>

#ifdef WIN32
#include <windows.h>
#endif


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

    ConfigWinRegImpl(const std::wstring& organization, const std::wstring& product)
    {
        std::wstring key = L"Software\\" + organization;


        HKEY hkey = NULL;
        RegCreateKeyExW(getRootKey(), key.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hkey, NULL);
        RegCloseKey(hkey);

        hkey = NULL;
        key += (L"\\" + product);
        RegCreateKeyExW(getRootKey(), key.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hkey, NULL);
        RegCloseKey(hkey);


        m_base_path = L"Software\\";
        m_base_path += organization;
        m_base_path += L'\\';
        m_base_path += product;

    }

    void setPath(const std::wstring& path)
    {
        m_path = path;

        // make sure it exists
        std::wstring key = getCurrentKeyPath();

        HKEY hkey = NULL;
        RegCreateKeyExW(getRootKey(), key.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hkey, NULL);
        RegCloseKey(hkey);
    }

    bool exists(const std::wstring& path)
    {
        std::wstring key = concatPath(getCurrentKeyPath(), path);

        HKEY hkey = NULL;
        if (ERROR_SUCCESS == RegOpenKeyExW(getRootKey(), key.c_str(), 0, KEY_READ, &hkey))
        {
            RegCloseKey(hkey);
            return true;
        }

        return false;
    }

    bool write(const std::wstring& path, long value)
    {
        std::wstring key, valname;
        getKeyAndValue(path, key, valname);

        HKEY hkey = NULL;
        if (ERROR_SUCCESS != RegCreateKeyExW(getRootKey(), key.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS, NULL, &hkey, NULL))
        {
            return false;
        }

        DWORD err, dwvalue;
        dwvalue = (DWORD)value;

        err = RegSetValueExW(hkey, valname.c_str(), 0, REG_DWORD, (const BYTE*)&dwvalue, sizeof(DWORD));

        RegCloseKey(hkey);

        return (err == ERROR_SUCCESS ? true : false);
    }

    bool write(const std::wstring& path, const std::wstring& value)
    {
        std::wstring key, valname;
        getKeyAndValue(path, key, valname);

        HKEY hkey = NULL;
        if (ERROR_SUCCESS != RegCreateKeyExW(getRootKey(), key.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS, NULL, &hkey, NULL))
        {
            return false;
        }

        DWORD err;
        err = RegSetValueExW(hkey, valname.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), value.length()+1);

        RegCloseKey(hkey);

        return (err == ERROR_SUCCESS ? true : false);
    }

    bool read(const std::wstring& path, std::wstring& value, const std::wstring& def)
    {
        std::wstring key, valname;
        getKeyAndValue(path, key, valname);

        
        HKEY hkey = NULL;
        if (ERROR_SUCCESS == RegOpenKeyExW(getRootKey(), key.c_str(), 0, KEY_QUERY_VALUE, &hkey))
        {
            RegCloseKey(hkey);
            value = def;
            return true;
        }

        // get value type
        DWORD err, dwtype, dwvalue, dwsize;
        err = RegQueryValueExW(hkey, valname.c_str(), 0, &dwtype, NULL, &dwsize);
        if (err != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            value = def;
            return false;
        }

        if (dwtype == REG_DWORD)
        {
            dwsize = sizeof(DWORD);
            err = RegQueryValueExW(hkey, valname.c_str(), 0, NULL, (BYTE*)&dwvalue, &dwsize);
            if (err != ERROR_SUCCESS)
            {
                RegCloseKey(hkey);
                value = def;
                return false;
            }

            RegCloseKey(hkey);
            value = kl::stdswprintf(L"%d", dwvalue);
            return true;
        }
         else if (dwtype == REG_SZ)
        {
            wchar_t* buf = new wchar_t[dwsize];
            err = RegQueryValueExW(hkey, valname.c_str(), 0, NULL, (BYTE*)buf, &dwsize);
            if (err != ERROR_SUCCESS)
            {
                delete[] buf;
                RegCloseKey(hkey);
                value = def;
                return false;
            }

            value = buf;
            delete[] buf;
            RegCloseKey(hkey);
            return true;
        }
         else
        {
            value = def;
            RegCloseKey(hkey);
            return false;
        }
    }

    bool read(const std::wstring& path, long* value, long def)
    {
        std::wstring key, valname;
        getKeyAndValue(path, key, valname);

        
        HKEY hkey = NULL;
        if (ERROR_SUCCESS == RegOpenKeyExW(getRootKey(), key.c_str(), 0, KEY_QUERY_VALUE, &hkey))
        {
            RegCloseKey(hkey);
            *value = def;
            return true;
        }

        // get value type
        DWORD err, dwtype, dwvalue, dwsize;
        err = RegQueryValueExW(hkey, valname.c_str(), 0, &dwtype, NULL, &dwsize);
        if (err != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            *value = def;
            return false;
        }

        if (dwtype == REG_DWORD)
        {
            dwsize = sizeof(DWORD);
            err = RegQueryValueExW(hkey, valname.c_str(), 0, NULL, (BYTE*)&dwvalue, &dwsize);
            if (err != ERROR_SUCCESS)
            {
                RegCloseKey(hkey);
                *value = def;
                return false;
            }

            RegCloseKey(hkey);
            *value = dwvalue;
            return true;
        }
         else if (dwtype == REG_SZ)
        {
            wchar_t* buf = new wchar_t[dwsize];
            err = RegQueryValueExW(hkey, valname.c_str(), 0, NULL, (BYTE*)buf, &dwsize);
            if (err != ERROR_SUCCESS)
            {
                delete[] buf;
                RegCloseKey(hkey);
                *value = def;
                return false;
            }

            *value = kl::wtoi(buf);
            delete[] buf;
            RegCloseKey(hkey);
            return true;
        }
         else
        {
            *value = def;
            RegCloseKey(hkey);
            return false;
        }
    }


    std::vector<std::wstring> getGroups(const std::wstring& path)
    {
        std::vector<std::wstring> res;

        std::wstring key = concatPath(getCurrentKeyPath(), path);
        
        HKEY hkey = NULL;
        if (ERROR_SUCCESS != RegOpenKeyExW(getRootKey(), key.c_str(), 0, KEY_ENUMERATE_SUB_KEYS, &hkey))
        {
            RegCloseKey(hkey);
            return res;
        }

        DWORD idx = 0;
        wchar_t name[512];
        while (ERROR_SUCCESS == RegEnumKeyW(hkey, idx++, name, 500))
        {
            res.push_back(name);
        }

        RegCloseKey(hkey);

        return res;
    }

    bool deleteGroup(const std::wstring& path)
    {
        std::vector<std::wstring> res;

        std::wstring key, valname;
        getKeyAndValue(path, key, valname);


        HKEY hkey = NULL;
        if (ERROR_SUCCESS != RegOpenKeyExW(getRootKey(), key.c_str(), 0, KEY_ALL_ACCESS, &hkey))
        {
            RegCloseKey(hkey);
            return false;
        }
        

        RegDeleteKeyW(hkey, valname.c_str());

        RegCloseKey(hkey);

        return true;
    }

private:

    std::wstring concatPath(const std::wstring& p1, const std::wstring& p2)
    {
        std::wstring result = p1 + L"\\" + p2;
        size_t i;
        for (i = 0; i < result.length(); ++i)
        {
            if (result[i] == '/')
                result[i] = '\\';
        }

        i = result.find(L"\\\\");
        while (i != result.npos)
        {
            result.erase(i, 1);
            i = result.find(L"\\\\");
        }
        
        if (result.length() > 0 && result[result.length()-1] == '\\')
        {
            return result.substr(0, result.length()-1);
        }
            
        return result;
    }

    void getKeyAndValue(const std::wstring& path, std::wstring& key, std::wstring& value)
    {
        std::wstring full = concatPath(getCurrentKeyPath(), path);

        if (full.find('\\') == full.npos)
        {
            key = full;
            value = L"";
            return;
        }

        key = kl::beforeLast(full, L'\\');
        value = kl::afterLast(full, L'//');
    }


    HKEY getRootKey()
    {
        return HKEY_CURRENT_USER;
    }

    std::wstring getCurrentKeyPath()
    {
        return concatPath(m_base_path, m_path);
    }

private:

    std::wstring m_base_path;
    std::wstring m_path;

};


#endif





Config::Config()
{
    m_impl = NULL;
}

Config::~Config()
{
    delete m_impl;
}

bool Config::init(const std::wstring& organization, const std::wstring& product)
{
#ifdef WIN32
    m_impl = new ConfigWinRegImpl(organization, product);
#endif

    return true;
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

