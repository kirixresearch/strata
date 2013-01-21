/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-05
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "tango.h"
#include "connectionstr.h"
#include <kl/portable.h>
#include <kl/crypt.h>


namespace xdcommon
{


ConnectionStr::ConnectionStr()
{
}

ConnectionStr::ConnectionStr(const std::wstring& str)
{
    parse(str);
}


void ConnectionStr::parse(const std::wstring& str)
{
    size_t next_pos = 0;
    
    m_map.clear();
    m_keys.clear();
    
    bool last = false;
    
    while (1)
    {
        std::wstring chunk, key, value;
        
        // -- get the next chunk --
        size_t delim = str.find(L';', next_pos);
        if (delim == str.npos)
        {
            last = true;
            chunk = str.substr(next_pos, str.length() - next_pos);
        }
         else
        {
            chunk.assign(str, next_pos, delim-next_pos);
        }
        next_pos = delim+1;

        kl::trim(chunk);
        if (chunk.empty())
            break;

        // -- get the key name --
        key = kl::beforeFirst(chunk, L'=');
        value = kl::afterFirst(chunk, L'=');

        kl::trim(key);
        kl::trim(value);
        
        if (key == value)
            value = L"";
            
        m_map[key] = value;
        
        // this separate vector of keys allows us to retain
        // the order of the values when we reconstruct a
        // connection string
        m_keys.push_back(key);
        
        if (last)
            break;
    }

}

// returns the value of the parameter, in lowercase
std::wstring ConnectionStr::getLowerValue(const std::wstring& param)
{
    std::wstring result = getValue(param);
    kl::makeLower(result);
    return result;
}

std::wstring ConnectionStr::getValue(const std::wstring& param)
{
    std::map<std::wstring, std::wstring, kl::cmp_nocase>::iterator it;
    it = m_map.find(param);
    
    if (it == m_map.end())
        return L"";
        
    return it->second;
}

bool ConnectionStr::getValueExist(const std::wstring& param)
{
    std::map<std::wstring, std::wstring, kl::cmp_nocase>::iterator it;
    it = m_map.find(param);
    return (it != m_map.end() ? true : false);
}


void ConnectionStr::setValue(const std::wstring& param, const std::wstring& value)
{
    std::map<std::wstring, std::wstring, kl::cmp_nocase>::iterator it;
    it = m_map.find(param);
    if (it != m_map.end())
    {
        it->second = value;
    }
     else
    {
        m_map[param] = value;
        m_keys.push_back(param);
    }
}


std::wstring ConnectionStr::getConnectionString()
{
    std::wstring result;
    
    std::vector<std::wstring>::iterator it;
    for (it = m_keys.begin(); it != m_keys.end(); ++it)
    {
        if (!result.empty())
            result += L";";
        result += *it;
        result += L"=";
        result += m_map[*it];
    }
    
    return result;
}





std::wstring urlToConnectionStr(const std::wstring& url)
{
    // manually parse url
    
    int url_sign = url.find(L"://");
    if (url_sign == -1)
        return L"";
        
    std::wstring protocol;
    std::wstring uid;
    std::wstring password;
    std::wstring host;
    std::wstring port;
    std::wstring rest;

    // get protocol
    protocol = kl::beforeFirst(url, L':');
    rest = url.substr(url_sign+3);
    
    // look for user name and/or password
    int at = rest.find(L"@");
    if (at != -1)
    {
        uid = rest.substr(0, at);
        
        int colon = uid.find(L":");
        if (colon != -1)
        {
            password = uid.substr(colon+1);
            uid = uid.substr(0, colon);
        }
        
        rest = rest.substr(at+1);
    }
    
    
    // find the remainder
    int last_slash = rest.find_last_of(L'/');
    
    if (last_slash == -1)
    {
        host = rest;
    }
     else
    {
        host = rest.substr(0, last_slash);
        rest = rest.substr(last_slash);
    }
    
    
    // find port number from host
    
    int colon = host.find(L':');
    if (colon != -1)
    {
        port = kl::afterFirst(host, L':');
        
        // test if port is a real number
        int p = kl::wtoi(port);
        wchar_t buf[100];
        swprintf(buf, 100, L"%d", p);
        if (port == buf)
        {
            host = kl::beforeFirst(host, L':');
        }
         else
        {
            // was not a port number
            port = L"";
        }
    }


    std::wstring cstr;
    
    kl::makeLower(protocol);
    if (protocol == L"mysql" ||
        protocol == L"mssql" ||
        protocol == L"oracle" ||
        protocol == L"odbc")
    {
        if (protocol == L"mysql")
        {
            #ifdef WIN32
            cstr += L"xdprovider=xdodbc;xddbtype=mysql";
            #else
            cstr += L"xdprovider=xddrizzle";
            //cstr += L"xdprovider=xdmysql";
            #endif
        }
        else if (protocol == L"mssql")
            cstr += L"xdprovider=xdodbc;xddbtype=mssql";
        else if (protocol == L"oracle")
            cstr += L"xdprovider=xdoracle";
        else if (protocol == L"odbc")
            cstr += L"xdprovider=xdodbc";
        
        cstr += L";host=";
        cstr += host;
        
        if (port.length() > 0)
        {
            cstr += L";port=";
            cstr += port;
        }
        
        if (rest.length() >= 2 && rest[0] == L'/')
        {
            cstr += L";database=";
            cstr += rest.substr(1);
        }
        
        if (uid.length() > 0)
        {
            cstr += L";user id=";
            cstr += uid;
        }
        
        if (password.length() > 0)
        {
            cstr += L";password=";
            cstr += password;
        }  
    }
     else if (protocol == L"xdfs")
    {
        cstr = L"xdprovider=xdfs";
    }
     else
    {
    }
    
    return cstr;
}



 

inline std::wstring getConnectionStringEncryptionKey()
{
    return std::wstring(L"jgk5]4X4$z(fq#[v8%43nFbgfer5^tnh").substr(7,8);
}


std::wstring decryptConnectionStringPassword(const std::wstring& connection_str)
{
    // decrypt password from string, if any
    std::wstring pw;
    ConnectionStr cstr(connection_str);

    bool changed = false;
    
    if (cstr.getValueExist(L"Pwd"))
    {
        pw = cstr.getValue(L"Pwd");
        pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
        cstr.setValue(L"Pwd", pw);
        changed = true;
    }
    
    if (cstr.getValueExist(L"Password"))
    {
        pw = cstr.getValue(L"Password");
        pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
        cstr.setValue(L"Password", pw);
        changed = true;
    }
    
    if (!changed)
        return connection_str;
        
    return cstr.getConnectionString();
}


std::wstring encryptConnectionStringPassword(const std::wstring& connection_str)
{
    // scramble password, if any
    std::wstring pw;
    ConnectionStr cstr(connection_str);

    bool changed = false;

    if (cstr.getValueExist(L"Pwd"))
    {
        pw = cstr.getValue(L"Pwd");
        pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
        cstr.setValue(L"Pwd", pw);
        changed = true;
    }
    
    if (cstr.getValueExist(L"Password"))
    {
        pw = cstr.getValue(L"Password");
        pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
        cstr.setValue(L"Password", pw);
        changed = true;
    }
    
    if (!changed)
        return connection_str;
        
    return cstr.getConnectionString();
}


};  // namespace xdcommon
