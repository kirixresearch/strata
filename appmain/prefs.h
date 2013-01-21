/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-05-06
 *
 */


#ifndef __APP_PREFS_H
#define __APP_PREFS_H




class AppPreference
{
public:

    enum
    {
        typeInvalid = 0,
        typeString = 1,
        typeLong = 2
    };

    wxString pref;
    int type;
    wxString strval;
    long longval;
};



xcm_interface IAppPreferences : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IAppPreferences")

public:

    virtual void init(const wxString& vendor_name, const wxString& app_name) = 0;

    virtual wxString getString(const wxString& pref, const wxString& default_val) = 0;
    virtual void setString(const wxString& pref, const wxString& new_value) = 0;

    virtual long getLong(const wxString& pref, long default_val) = 0;
    virtual void setLong(const wxString& pref, long new_value) = 0;

    virtual bool getBoolean(const wxString& pref, bool default_val) = 0;
    virtual void setBoolean(const wxString& pref, bool new_value) = 0;
    
    virtual wxColor getColor(const wxString& pref, const wxColour& default_val) = 0;
    virtual void setColor(const wxString& pref, const wxColor& new_value) = 0;

    virtual bool remove(const wxString& pref) = 0;
    virtual bool exists(const wxString& pref) = 0;
    virtual int getType(const wxString& pref) = 0;
    
    virtual void getAll(std::vector<AppPreference>& prefs) = 0;
    
    virtual void flush() = 0;
};

XCM_DECLARE_SMARTPTR(IAppPreferences)


IAppPreferencesPtr createAppPreferencesObject();
IAppPreferencesPtr createMemoryAppPreferencesObject();


#endif
