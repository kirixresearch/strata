/*!
 *
 * Copyright (c) 2003-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __CFW_PREFS_PRIVATE_H
#define __CFW_PREFS_PRIVATE_H


#include <map>


namespace cfw
{

// -- app preferences implementation --





class AppPreferences : public IAppPreferences
{

    XCM_CLASS_NAME("cfw.AppPreferences")
    XCM_BEGIN_INTERFACE_MAP(AppPreferences)
        XCM_INTERFACE_ENTRY(cfw::IAppPreferences)
    XCM_END_INTERFACE_MAP()
    
public:
    
    AppPreferences();
    virtual ~AppPreferences();

    void init(const wxString& vendor_name, const wxString& app_name);
    wxString getString(const wxString& pref, const wxString& default_val);
    void setString(const wxString& pref, const wxString& new_value);
    long getLong(const wxString& pref, long default_val);
    void setLong(const wxString& pref, long new_value);
    bool getBoolean(const wxString& pref, bool default_val);
    void setBoolean(const wxString& pref, bool new_value);
    wxColor getColor(const wxString& pref, const wxColour& default_val);
    void setColor(const wxString& pref, const wxColor& new_value);

    void getAll(std::vector<AppPreference>& prefs);
    
    void refresh();
    void flush();

    bool remove(const wxString& pref);
    bool exists(const wxString& pref);
    int getType(const wxString& pref);

private:

    AppPreference* findPref(const wxString& pref)
    {
        std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
        it = m_map.find(pref);
        if (it == m_map.end())
            return NULL;
        return &(it->second);
    }
    
    wxString getVendorRegKey() const;
    wxString getRegKey() const;
     
private:

    std::map<wxString, AppPreference, std::less<wxString> > m_map;
    bool m_dirty;
    wxString m_vendor_name;
    wxString m_app_name;
};




class MemoryAppPreferences : public IAppPreferences
{
    XCM_CLASS_NAME("cfw.MemoryAppPreferences")
    XCM_BEGIN_INTERFACE_MAP(MemoryAppPreferences)
        XCM_INTERFACE_ENTRY(cfw::IAppPreferences)
    XCM_END_INTERFACE_MAP()
    
public:
    
    MemoryAppPreferences();
    virtual ~MemoryAppPreferences();

    void init(const wxString& vendor_name, const wxString& app_name);
    wxString getString(const wxString& pref, const wxString& default_val);
    void setString(const wxString& pref, const wxString& new_value);
    long getLong(const wxString& pref, long default_val);
    void setLong(const wxString& pref, long new_value);
    bool getBoolean(const wxString& pref, bool default_val);
    void setBoolean(const wxString& pref, bool new_value);
    wxColor getColor(const wxString& pref, const wxColour& default_val);
    void setColor(const wxString& pref, const wxColor& new_value);

    void getAll(std::vector<AppPreference>& prefs);
    
    void refresh();
    void flush();

    bool remove(const wxString& pref);
    bool exists(const wxString& pref);
    int getType(const wxString& pref);

private:

    AppPreference* findPref(const wxString& pref)
    {
        std::map<wxString, AppPreference, std::less<wxString> >::iterator it;
        it = m_map.find(pref);
        if (it == m_map.end())
            return NULL;
        return &(it->second);
    }
     
private:

    std::map<wxString, AppPreference, std::less<wxString> > m_map;
};





};  // namespace cfw




#endif

