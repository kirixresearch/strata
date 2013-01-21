/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif


#include "appmain.h"
#include <wx/msw/registry.h>
#include <xcm/xcm.h>
#include "prefs.h"
#include "prefs_private.h"

IAppPreferencesPtr createAppPreferencesObject()
{
    return static_cast<IAppPreferences*>(new AppPreferences);
}

IAppPreferencesPtr createMemoryAppPreferencesObject()
{
    return static_cast<IAppPreferences*>(new MemoryAppPreferences);
}


AppPreferences::AppPreferences()
{
}

AppPreferences::~AppPreferences()
{
    flush();
}

void AppPreferences::init(const wxString& vendor_name, const wxString& app_name)
{
    m_vendor_name = vendor_name;
    m_app_name = app_name;
    
#ifdef __WXMSW__
    wxRegKey key1(getVendorRegKey());
    if (!key1.Exists())
        key1.Create();
        
    wxRegKey key2(getRegKey());
    if (!key2.Exists())
        key2.Create();
#endif
    
    refresh();
}

wxString AppPreferences::getVendorRegKey() const
{
    wxASSERT_MSG(m_vendor_name.Length() > 0, wxT("Please call AppPreferences::init() first"));
    wxString key = wxT("HKEY_CURRENT_USER\\Software\\");
    key += m_vendor_name;
    return key;
    
}
wxString AppPreferences::getRegKey() const
{
    wxASSERT_MSG(m_vendor_name.Length() > 0, wxT("Please call AppPreferences::init() first"));
    wxASSERT_MSG(m_app_name.Length() > 0, wxT("Please call AppPreferences::init() first"));
    
    wxString key = wxT("HKEY_CURRENT_USER\\Software\\");
    key += m_vendor_name;
    key += wxT("\\");
    key += m_app_name;
    key += wxT("\\Settings");
    return key;
}


wxString AppPreferences::getString(const wxString& pref,
                                   const wxString& default_val)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Read))
    {
        if (key.GetValueType(pref) == wxRegKey::Type_String)
        {
            wxString value;
            if (key.QueryValue(pref, value))
                return value;
        }
    }
    
    return default_val;

#else
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeString)
        return default_val;

    return ppref->strval;
#endif
}

void AppPreferences::setString(const wxString& pref,
                               const wxString& new_value)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Write))
        key.SetValue(pref, new_value);
        
#else

    m_dirty = true;

    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.strval = new_value;
        new_pref.type = AppPreference::typeString;
        new_pref.longval = 0;
        m_map[pref] = new_pref;
        return;
    }

    ppref->strval = new_value;
    ppref->type = AppPreference::typeString;
    
#endif

}

long AppPreferences::getLong(const wxString& pref, long default_val)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Read))
    {
        if (key.GetValueType(pref) == wxRegKey::Type_Dword)
        {
            long value;
            if (key.QueryValue(pref, &value))
                return value;
        }
    }
    
    return default_val;
#else

    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
        return default_val;

    return ppref->longval;

#endif

}

void AppPreferences::setLong(const wxString& pref, long new_value)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Write))
        key.SetValue(pref, new_value);
#else
    m_dirty = true;

    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = new_value;
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }

    ppref->longval = new_value;
    ppref->type = AppPreference::typeLong;
#endif
}

bool AppPreferences::getBoolean(const wxString& pref, bool default_val)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Read))
    {
        if (key.GetValueType(pref) == wxRegKey::Type_Dword)
        {
            long value;
            if (key.QueryValue(pref, &value))
                return value ? true : false;
        }
    }
    
    return default_val;
#else
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
    {
        return default_val;
    }

    return (ppref->longval ? true : false);
#endif

}

void AppPreferences::setBoolean(const wxString& pref, bool new_value)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Write))
    {
        long l = new_value ? 1 : 0;
        key.SetValue(pref, l);
    }
#else
    m_dirty = true;

    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = (new_value ? 1 : 0);
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }

    ppref->longval = (new_value ? 1 : 0);
    ppref->type = AppPreference::typeLong;
#endif
}

wxColor AppPreferences::getColor(const wxString& pref,
                                 const wxColour& default_val)
{
    long longval = 0;
    
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Read))
    {
        if (key.GetValueType(pref) == wxRegKey::Type_Dword)
        {
            if (!key.QueryValue(pref, &longval))
                return default_val;
        }
         else
        {
            return default_val;
        }
    }
     else
    {
        return default_val;
    }
#else
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
        return default_val;
    longval = ppref->longval;
#endif

    wxColor retval;
    retval.Set((longval >> 16) & 0xff,
               (longval >> 8) & 0xff,
               (longval) & 0xff);
    return retval;
}

void AppPreferences::setColor(const wxString& pref, const wxColor& new_value)
{
    long newlong = (new_value.Red() << 16) |
                   (new_value.Green() << 8) |
                    new_value.Blue();

#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (key.Open(wxRegKey::Write))
        key.SetValue(pref, newlong);
#else
    m_dirty = true;

    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = newlong;
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }
    
    ppref->longval = newlong;
    ppref->type = AppPreference::typeLong;
#endif

}

void AppPreferences::refresh()
{
#ifndef __WXMSW__
    m_map.clear();
    
    if (m_app_name.IsEmpty() ||
        m_vendor_name.IsEmpty())
    {
        // if you are here, you forgot to call init() to specify
        // your vendor name and application name
        return;
    }
    
    wxConfig* config = new wxConfig(m_app_name, m_vendor_name);

    if (config->Exists(wxT("/Settings")))
    {
        config->SetPath(wxT("/Settings"));

        AppPreference pref;

        long counter;
        wxString str;
        wxConfigBase::EntryType type;
        bool group_cont = config->GetFirstEntry(str, counter);
        while (group_cont)
        {
            pref.type = 0;
            pref.longval = 0;
            pref.strval.Clear();

            type = config->GetEntryType(str);

            if (type == wxConfigBase::Type_Integer)
            {
                pref.type = AppPreference::typeLong;
                config->Read(str, &pref.longval, 0);
                
                // store it also as a string; this is necessary
                // for the non-Windows versions of wxConfig*
                // because they do not store data types
                pref.strval.Format(wxT("%ld"), pref.longval);
            }
             else
            {
                wxString val;

                pref.type = AppPreference::typeString;
                config->Read(str, &val, wxT(""));


                pref.strval = val;

                if (val.Left(6) == wxT("$LONG("))
                {
                    val = val.AfterFirst(wxT('('));
                    val = val.BeforeLast(wxT(')'));
                    pref.type = AppPreference::typeLong;
                    pref.longval = wxAtol(val);
                    pref.strval.Printf(wxT("%ld"), pref.longval);
                }
#ifdef __WXGTK__
                 else
                {
                    // try to store it also as a number; this is necessary
                    // for the non-Windows versions of wxConfig*
                    // because they do not store data types --

                    wxString temps = pref.strval;
                    temps.Trim(false);
                    temps.Trim(true);
                    long templ = wxAtol(temps);
                    wxString temps2;
                    temps2 = wxString::Format(wxT("%ld"), templ);
                    if (temps2 == temps)
                    {
                        pref.strval = temps;
                        pref.longval = templ;
                        pref.type = AppPreference::typeLong;
                    }
                }
#endif

            }

            m_map[str] = pref;
            
            group_cont = config->GetNextEntry(str, counter);
        }
    }

    delete config;

    m_dirty = false;
#endif
}

void AppPreferences::flush()
{
#ifndef __WXMSW__
    if (!m_dirty)
        return;

    if (m_app_name.IsEmpty() ||
        m_vendor_name.IsEmpty())
    {
        // if you are here, you forgot to call init() to specify
        // your vendor name and application name
        return;
    }

    wxConfig* config = new wxConfig(m_app_name, m_vendor_name);
    config->SetPath(wxT("/Settings"));

    std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
    AppPreference* pref;
    for (it = m_map.begin(); it != m_map.end(); ++it)
    {
        pref = &(it->second);


#ifdef __WXMSW__
        switch (pref->type)
        {
            case AppPreference::typeString:
                config->Write(it->first, pref->strval);
                break;

            case AppPreference::typeLong:
                config->Write(it->first, pref->longval);
                break;
        }
#else
        wxString str;

        switch (pref->type)
        {
            case AppPreference::typeString:
                str = pref->strval;
                break;

            case AppPreference::typeLong:
                str.Printf(wxT("$LONG(%ld)"), pref->longval);
                break;
        }

        config->Write(it->first, str);
#endif

    }

    delete config;
#endif
}

bool AppPreferences::remove(const wxString& pref)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (!key.Open(wxRegKey::Write))
        return false;
    
    return key.DeleteValue(pref);

#else
    AppPreference* ppref = findPref(pref);
    if (!ppref)
        return false;

    if (m_app_name.IsEmpty() ||
        m_vendor_name.IsEmpty())
    {
        // if you are here, you forgot to call
        // init() to specify your vendor name and
        // application name
        return false;
    }

    wxConfig* config = new wxConfig(m_app_name, m_vendor_name);
    config->SetPath(wxT("/Settings"));

    std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
    it = m_map.find(pref);

    if (it != m_map.end())
    {
        m_map.erase(it);
    }    

    bool retval = config->DeleteEntry(pref, false);

    delete config;
    return retval;
#endif
}

bool AppPreferences::exists(const wxString& pref)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (!key.Open(wxRegKey::Read))
        return false;
    
    return key.HasValue(pref);
    
#else
    AppPreference* ppref = findPref(pref);
    if (!ppref)
        return false;

    return true;
#endif
}

int AppPreferences::getType(const wxString& pref)
{
#ifdef __WXMSW__
    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (!key.Open(wxRegKey::Read))
        return false;
    
    switch (key.GetValueType(pref))
    {
        default:
        case wxRegKey::Type_None:          return AppPreference::typeInvalid;
        case wxRegKey::Type_String:
        case wxRegKey::Type_Expand_String: return AppPreference::typeString;
        case wxRegKey::Type_Dword:         return AppPreference::typeLong;
    }
    
    return AppPreference::typeInvalid;
#else
    AppPreference* ppref = findPref(pref);
    if (!ppref)
        return AppPreference::typeInvalid;

    return ppref->type;
#endif

}


void AppPreferences::getAll(std::vector<AppPreference>& prefs)
{
    prefs.clear();

#ifdef __WXMSW__

    wxLogNull log;
    
    wxRegKey key(getRegKey());
    if (!key.Open(wxRegKey::Read))
        return;

    long i;
    wxString val;
    if (!key.GetFirstValue(val, i))
        return;
    do
    {
        if (val.Length() == 0)
            continue;
        
        AppPreference pref;
        pref.pref = val;
        
        int type = key.GetValueType(val);
        if (type == wxRegKey::Type_String || type == wxRegKey::Type_Expand_String)
        {
            pref.type = AppPreference::typeString;
            if (key.QueryValue(val, pref.strval))
                prefs.push_back(pref);
        }
         else if (type == wxRegKey::Type_Dword)
        {
            pref.type = AppPreference::typeLong;
            if (key.QueryValue(val, &pref.longval))
                prefs.push_back(pref);
        }
        
    } while (key.GetNextValue(val, i));
    
#else
    std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
    for (it = m_map.begin(); it != m_map.end(); ++it)
    {
        AppPreference pref = it->second;
        pref.pref = it->first;
        prefs.push_back(pref);
    }
#endif
}








// MemoryAppPreferences is a class that stores preferences in memory,
// and doesn't serialize/save them anywhere on disk

MemoryAppPreferences::MemoryAppPreferences()
{
}

MemoryAppPreferences::~MemoryAppPreferences()
{
}

void MemoryAppPreferences::init(const wxString& vendor_name, const wxString& app_name)
{
}

wxString MemoryAppPreferences::getString(const wxString& pref,
                                   const wxString& default_val)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeString)
        return default_val;

    return ppref->strval;
}

void MemoryAppPreferences::setString(const wxString& pref,
                               const wxString& new_value)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.strval = new_value;
        new_pref.type = AppPreference::typeString;
        new_pref.longval = 0;
        m_map[pref] = new_pref;
        return;
    }

    ppref->strval = new_value;
    ppref->type = AppPreference::typeString;
}

long MemoryAppPreferences::getLong(const wxString& pref, long default_val)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
    {
        return default_val;
    }

    return ppref->longval;
}

void MemoryAppPreferences::setLong(const wxString& pref, long new_value)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = new_value;
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }

    ppref->longval = new_value;
    ppref->type = AppPreference::typeLong;
}

bool MemoryAppPreferences::getBoolean(const wxString& pref, bool default_val)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
    {
        return default_val;
    }

    return (ppref->longval ? true : false);
}

void MemoryAppPreferences::setBoolean(const wxString& pref, bool new_value)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = (new_value ? 1 : 0);
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }

    ppref->longval = (new_value ? 1 : 0);
    ppref->type = AppPreference::typeLong;
}

wxColor MemoryAppPreferences::getColor(const wxString& pref,
                                 const wxColour& default_val)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref || ppref->type != AppPreference::typeLong)
    {
        return default_val;
    }

    wxColor retval;
    retval.Set((ppref->longval >> 16) & 0xff,
               (ppref->longval >> 8) & 0xff,
               (ppref->longval) & 0xff);
    return retval;
}

void MemoryAppPreferences::setColor(const wxString& pref, const wxColor& new_value)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref)
    {
        AppPreference new_pref;
        new_pref.longval = (new_value.Red() << 16) |
                             (new_value.Green() << 8) |
                             new_value.Blue();
        new_pref.type = AppPreference::typeLong;
        m_map[pref] = new_pref;
        return;
    }

    ppref->longval = (new_value.Red() << 16) |
                       (new_value.Green() << 8) |
                       new_value.Blue();
    ppref->type = AppPreference::typeLong;
}

void MemoryAppPreferences::refresh()
{
}

void MemoryAppPreferences::flush()
{
}

bool MemoryAppPreferences::remove(const wxString& pref)
{   
    std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
    it = m_map.find(pref);

    if (it != m_map.end())
    {
        m_map.erase(it);
        return true;
    }    

    return false;
}

bool MemoryAppPreferences::exists(const wxString& pref)
{   
    AppPreference* ppref = findPref(pref);
    if (!ppref)
        return false;

    return true;
}

int MemoryAppPreferences::getType(const wxString& pref)
{
    AppPreference* ppref = findPref(pref);
    if (!ppref)
        return AppPreference::typeInvalid;

    return ppref->type;
}


void MemoryAppPreferences::getAll(std::vector<AppPreference>& prefs)
{
    prefs.clear();
    
    std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
    for (it = m_map.begin(); it != m_map.end(); ++it)
    {
        AppPreference pref = it->second;
        pref.pref = it->first;
        prefs.push_back(pref);
    }
}

