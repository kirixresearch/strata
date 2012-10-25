/*!
 *
 * Copyright (c) 2008-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-10-23
 *
 */


#ifndef __APP_APPCONFIG_H
#define __APP_APPCONFIG_H


#include <vector>
#include <string>

// interface delcaration
xcm_interface IAppConfig;
XCM_DECLARE_SMARTPTR(IAppConfig)


xcm_interface IAppConfig : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IAppConfig")

public:

    virtual void setPath(const std::wstring& path) = 0;

    virtual bool exists(const std::wstring& path) = 0;

    virtual bool write(const std::wstring& path, const std::wstring& value) = 0;
    virtual bool write(const std::wstring& path, long value) = 0;
    virtual bool write(const std::wstring& path, bool value) { return write(path, (long)(value ? 1 : 0)); }

    virtual bool read(const std::wstring& path, std::wstring& value, const std::wstring& def = L"") = 0;
    virtual bool read(const std::wstring& path, long* value, long def = 0) = 0;
    virtual bool read(const std::wstring& path, bool* value, bool def = false)
    {
        long l;
        bool res = read(path, &l, def ? 1 : 0);
        *value = (l == 0 ? false : true);
        return res;
    }

    virtual std::vector<std::wstring> getGroups(const std::wstring& path = L"") = 0;
    virtual bool deleteGroup(const std::wstring& path = L"") = 0;
};



IAppConfigPtr createAppConfigObject();



#endif  // __APP_APPCONFIG_H
