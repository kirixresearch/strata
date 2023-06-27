/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-04-05
 *
 */


#include <xd/xd.h>
#include "connectionstr.h"
#include <kl/portable.h>
#include <kl/crypt.h>
#include <kl/string.h>

namespace xdcommon
{




std::wstring urlToConnectionStr(const std::wstring& url)
{
    // manually parse url
    
    size_t url_sign = url.find(L"://");
    if (url_sign == url.npos)
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
    size_t at = rest.find(L"@");
    if (at != rest.npos)
    {
        uid = rest.substr(0, at);
        
        size_t colon = uid.find(L":");
        if (colon != uid.npos)
        {
            password = uid.substr(colon+1);
            uid = uid.substr(0, colon);
        }
        
        rest = rest.substr(at+1);
    }
    
    
    // find the remainder
    size_t last_slash = rest.find_last_of(L'/');
    
    if (last_slash == rest.npos)
    {
        host = rest;
    }
     else
    {
        host = rest.substr(0, last_slash);
        rest = rest.substr(last_slash);
    }
    
    
    // find port number from host
    
    size_t colon = host.find(L':');
    if (colon != host.npos)
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
            cstr += L"xdprovider=xdmysql";
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
    if (connection_str.length() == 0)
        return connection_str;

    // decrypt password from string, if any
    std::wstring pw;
    xd::ConnectionString cstr(connection_str);

    bool changed = false;
    
    if (cstr.getValueExist(L"Pwd"))
    {
        pw = cstr.getValue(L"Pwd");
        if (pw.length() > 0)
        {
            pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Pwd", pw);
            changed = true;
        }
    }
    
    if (cstr.getValueExist(L"Password"))
    {
        pw = cstr.getValue(L"Password");
        if (pw.length() > 0)
        {
            pw = kl::decryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Password", pw);
            changed = true;
        }
    }
    
    if (!changed)
    {
        return connection_str;
    }
        
    return cstr.getConnectionString();
}


std::wstring encryptConnectionStringPassword(const std::wstring& connection_str)
{
    if (connection_str.length() == 0)
        return connection_str;

    // scramble password, if any
    std::wstring pw;
    xd::ConnectionString cstr(connection_str);

    bool changed = false;

    if (cstr.getValueExist(L"Pwd"))
    {
        pw = cstr.getValue(L"Pwd");
        if (pw.length() > 0)
        {
            pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Pwd", pw);
            changed = true;
        }
    }
    
    if (cstr.getValueExist(L"Password"))
    {
        pw = cstr.getValue(L"Password");
        if (pw.length() > 0)
        {
            pw = kl::encryptString(pw, getConnectionStringEncryptionKey());
            cstr.setValue(L"Password", pw);
            changed = true;
        }
    }
    
    if (!changed)
    {
        return connection_str;
    }
        
    return cstr.getConnectionString();
}


};  // namespace xdcommon
