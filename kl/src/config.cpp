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

    ConfigWinRegImpl()
    {
    }

    void setPath(const std::wstring& path)
    {
        m_path = path;
    }

    bool exists(const std::wstring& path)
    {
        std::wstring key = concatPath(m_path, path);

        HKEY hkey = NULL;
        if (ERROR_SUCCESS == RegOpenKeyEx(getRootKey(), key.c_str(), 0, KEY_READ, &hkey))
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
        if (ERROR_SUCCESS == RegOpenKeyEx(getRootKey(), key.c_str(), 0, KEY_QUERY_VALUE, &hkey))
        {
            RegCloseKey(hkey);
            value = def;
            return true;
        }

        // get value type
        DWORD err, dwtype, dwvalue, dwsize;
        err = RegQueryValueEx(hkey, valname.c_str(), 0, &dwtype, NULL, &dwsize);
        if (err != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            value = def;
            return false;
        }

        if (dwtype == REG_DWORD)
        {
            dwsize = sizeof(DWORD);
            err = RegQueryValueEx(hkey, valname.c_str(), 0, NULL, (BYTE*)&dwvalue, &dwsize);
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
            err = RegQueryValueEx(hkey, valname.c_str(), 0, NULL, (BYTE*)buf, &dwsize);
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
        if (ERROR_SUCCESS == RegOpenKeyEx(getRootKey(), key.c_str(), 0, KEY_QUERY_VALUE, &hkey))
        {
            RegCloseKey(hkey);
            *value = def;
            return true;
        }

        // get value type
        DWORD err, dwtype, dwvalue, dwsize;
        err = RegQueryValueEx(hkey, valname.c_str(), 0, &dwtype, NULL, &dwsize);
        if (err != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            *value = def;
            return false;
        }

        if (dwtype == REG_DWORD)
        {
            dwsize = sizeof(DWORD);
            err = RegQueryValueEx(hkey, valname.c_str(), 0, NULL, (BYTE*)&dwvalue, &dwsize);
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
            err = RegQueryValueEx(hkey, valname.c_str(), 0, NULL, (BYTE*)buf, &dwsize);
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
    }

    bool deleteGroup(const std::wstring& path)
    {
    }

private:

    std::wstring concatPath(const std::wstring& p1, const std::wstring& p2)
    {
        std::wstring result = p1 + p2;
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
        std::wstring full = concatPath(m_path, path);

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

