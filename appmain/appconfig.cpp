/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2012-10-23
 *
 */



#include "appmain.h"
#include "appconfig.h"
#include <wx/config.h>


#ifdef __WXMSW__
class AppConfigRegImpl : public IAppConfig
{
    XCM_CLASS_NAME("appmain.AppConfigRegImpl")
    XCM_BEGIN_INTERFACE_MAP(AppConfigRegImpl)
        XCM_INTERFACE_ENTRY(IAppConfig)
    XCM_END_INTERFACE_MAP()

public:

    AppConfigRegImpl()
    {
        m_reg_toplevel = wxRegKey::HKCU;

        m_reg_root = L"Software\\";
        m_reg_root += APP_CONFIG_KEY;
        m_reg_root += L"\\";
        m_reg_root += APP_COMPANY_KEY;
    }

    virtual ~AppConfigRegImpl()
    {
    }

    void setRegRoot(wxRegKey::StdKey top_level, const std::wstring& reg_root)
    {
        m_reg_toplevel = top_level;
        m_reg_root = reg_root;
    }

    void setPath(const std::wstring& path)
    {
        if (path.length() == 0 || path[0] == '/')
        {
            m_cwd = path;
        }
         else
        {
            wchar_t last_char = m_cwd[m_cwd.length() - 1];
            if (last_char != '\\' && last_char != '/')
                m_cwd += '\\';
            m_cwd += path;
        }
        
        size_t i, len = m_cwd.length();
        for (i = 0; i < len; ++i)
        {
            if (m_cwd[i] == '/')
                m_cwd[i] = '\\';
        }

        if (m_cwd.length() > 0 && m_cwd[0] == '\\')
            m_cwd.erase(0, 1);
    }

    bool exists(const std::wstring& path)
    {
        wxLogNull lognull;

        std::wstring regkey, regvalue;

        getGroupLocation(path, regkey);

        {
            wxRegKey key(m_reg_toplevel, regkey);
            if (key.Exists())
                return true;
        }

        getLocation(path, regkey, regvalue);

        {
            wxRegKey key(m_reg_toplevel, regkey);
            if (!key.Open(wxRegKey::Read))
                return false;

            return key.HasValue(regvalue);
        }
    }

    bool write(const std::wstring& path, const std::wstring& value)
    {
        wxLogNull lognull;

        std::wstring regkey, regvalue;
        getLocation(path, regkey, regvalue);

        wxRegKey key(m_reg_toplevel, regkey);

        if (!key.Exists())
        {
            if (!key.Create())
                return false;
        }

        if (key.Open(wxRegKey::Write))
        {
            key.SetValue(regvalue, value);
            return true;
        }
         else
        {
            return false;
        }
    }

    bool write(const std::wstring& path, long value)
    {
        wxLogNull lognull;

        std::wstring regkey, regvalue;
        getLocation(path, regkey, regvalue);

        wxRegKey key(m_reg_toplevel, regkey);

        if (!key.Exists())
        {
            if (!key.Create())
                return false;
        }

        if (key.Open(wxRegKey::Write))
        {
            key.SetValue(regvalue, value);
            return true;
        }
         else
        {
            return false;
        }
    }

    bool read(const std::wstring& path, std::wstring& value, const std::wstring& def)
    {
        wxLogNull lognull;

        std::wstring regkey, regvalue;
        getLocation(path, regkey, regvalue);

        wxRegKey key(m_reg_toplevel, regkey);
        if (!key.Open(wxRegKey::Read))
            return false;

        wxString res;
        if (key.QueryValue(regvalue, res))
        {
            value = towstr(res);
            return true;
        }
         else
        {
            value = def;
            return false;
        }
    }

    bool read(const std::wstring& path, long* value, long def)
    {
        wxLogNull lognull;

        std::wstring regkey, regvalue;
        getLocation(path, regkey, regvalue);

        wxRegKey key(m_reg_toplevel, regkey);
        if (!key.Open(wxRegKey::Read))
            return false;

        long res;
        if (key.QueryValue(regvalue, &res))
        {
            *value = res;
            return true;
        }
         else
        {
            *value = def;
            return false;
        }
    }

    std::vector<std::wstring> getGroups(const std::wstring& path)
    {
        wxLogNull lognull;

        std::vector<std::wstring> res;

        std::wstring regkey;
        getGroupLocation(path, regkey);


        wxRegKey key(m_reg_toplevel, regkey);
        if (!key.Open(wxRegKey::Read))
            return res;
            
        wxString name;
        long counter;
        bool cont = key.GetFirstKey(name, counter);
        while (cont)
        {
            res.push_back(towstr(name));
            cont = key.GetNextKey(name, counter);
        }
        

        return res;
    }

    void getLocation(const std::wstring& path, std::wstring& regkey, std::wstring& regvalue)
    {
        getGroupLocation(path, regkey);

        regvalue = kl::afterLast(regkey, '\\');
        regkey = kl::beforeLast(regkey, '\\');
    }

    void getGroupLocation(const std::wstring& path, std::wstring& regkey)
    {
        regkey = m_reg_root;

        if (m_cwd.length() > 0)
        {
            if (m_cwd[0] != '\\')
                regkey += L"\\";
            regkey += m_cwd;
        }

        if (path.length() > 0)
        {
            if (path[0] != '\\')
                regkey += L"\\";
            regkey += path;
        }
    }
    
    bool deleteGroup(const std::wstring& path)
    {
        wxLogNull lognull;

        std::wstring regkey;
        getGroupLocation(path, regkey);

        wxRegKey key(m_reg_toplevel, regkey);
        return key.DeleteSelf();
    }

    bool flush()
    {
        return true;
    }

protected:

    wxRegKey::StdKey m_reg_toplevel;
    std::wstring m_reg_root;

    std::wstring m_base_path;
    std::wstring m_cwd;

};
#endif



class AppConfigGenericImpl : public IAppConfig
{
    XCM_CLASS_NAME("appmain.AppConfigGenericImpl")
    XCM_BEGIN_INTERFACE_MAP(AppConfigGenericImpl)
        XCM_INTERFACE_ENTRY(IAppConfig)
    XCM_END_INTERFACE_MAP()

public:

    AppConfigGenericImpl()
    {
        m_config = new wxConfig(APP_CONFIG_KEY, APP_COMPANY_KEY);
    }

    virtual ~AppConfigGenericImpl()
    {
        delete m_config;
    }

    void setPath(const std::wstring& path)
    {
        m_config->SetPath(path);
    }

    bool exists(const std::wstring& path)
    {
        return m_config->Exists(path);
    }

    bool write(const std::wstring& path, const std::wstring& value)
    {
        return m_config->Write(path, value.c_str());
    }

    bool write(const std::wstring& path, long value)
    {
        return m_config->Write(path, value);
    }

    bool read(const std::wstring& path, std::wstring& value, const std::wstring& def)
    {
        wxString wxval;
        bool res = m_config->Read(path, &wxval, def);
        value = towstr(wxval);
        return res;
    }

    bool read(const std::wstring& path, long* value, long def)
    {
        return m_config->Read(path, value, def);
    }

    std::vector<std::wstring> getGroups(const std::wstring& path)
    {
        std::vector<std::wstring> res;

        wxString str;
        long counter;
        bool group_cont = m_config->GetFirstGroup(str, counter);
        while (group_cont)
        {
            res.push_back(towstr(str));
            group_cont = m_config->GetNextGroup(str, counter);
        }

        return res;
    }


    bool deleteGroup(const std::wstring& path)
    {
        return m_config->DeleteGroup(path);
    }

    bool flush()
    {
        return m_config->Flush();
    }


private:

    wxConfig* m_config;
};



IAppConfigPtr createAppConfigObject()
{
    if (g_app->isServiceConfig())
    {
        // access LocalSystem's HKCU\Software\<company>\<product> tree

#ifdef __WXMSW__
        AppConfigRegImpl* res = new AppConfigRegImpl;
        if (true)
        {
            std::wstring reg_root;

            reg_root = L"S-1-5-18\\Software\\";
            reg_root += APP_CONFIG_KEY;
            reg_root += L"\\";
            reg_root += APP_COMPANY_KEY;

            res->setRegRoot(wxRegKey::HKUSR, reg_root);
        };
#else
        AppConfigGenericImpl* res = new AppConfigGenericImpl;
#endif

        return static_cast<IAppConfig*>(res);
    }
     else
    {
        // normal HKCU storage using wxConfig

        AppConfigGenericImpl* res = new AppConfigGenericImpl;
        return static_cast<IAppConfig*>(res);
    }
}



