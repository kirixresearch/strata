/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  wxWebConnect Embedded Web Browser Control Library
 * Author:   Benjamin I. Williams
 * Created:  2007-04-23
 *
 */


#include <wx/wx.h>
#include "webcontrol.h"
#include "nsinclude.h"


///////////////////////////////////////////////////////////////////////////////
//  wxWebPreferences class implementation
///////////////////////////////////////////////////////////////////////////////


wxWebPreferences::wxWebPreferences()
{
}

bool wxWebPreferences::GetBoolPref(const wxString& name)
{
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    wxASSERT(!prefs.empty());
    if (prefs.empty())
        return false;
        
   // PRBool val;
    bool val;
    if (NS_FAILED(prefs->GetBoolPref((const char*)name.mbc_str(), &val)))
        return false;
        
   // return (val == PR_TRUE ? true : false);
    return val;
}

wxString wxWebPreferences::GetStringPref(const wxString& name)
{
    wxString val;
    
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    wxASSERT(!prefs.empty());
    if (prefs.empty())
        return val;

    char* cstr = NULL;
    
    if (NS_FAILED(prefs->GetCharPref((const char*)name.mbc_str(), &cstr)))
        return val;
    
    if (cstr)
    {
        val = wxString::From8BitData(cstr);
        NS_Free(cstr);
    }
    
    return val;
}

int wxWebPreferences::GetIntPref(const wxString& name)
{
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    wxASSERT(!prefs.empty());
    if (prefs.empty())
        return 0;
    
    int val = 0;
    
    if (NS_FAILED(prefs->GetIntPref((const char*)name.mbc_str(), &val)))
        return 0;
        
    return val;
}

void wxWebPreferences::SetIntPref(const wxString& name, int value)
{
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    if (prefs.empty())
        return;
        
    prefs->SetIntPref((const char*)name.mbc_str(), value);
}

void wxWebPreferences::SetStringPref(const wxString& name, const wxString& value)
{
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    if (prefs.empty())
        return;

    prefs->SetCharPref((const char*)name.mbc_str(), (const char*)value.mbc_str());
}

void wxWebPreferences::SetBoolPref(const wxString& name, bool value)
{
    ns_smartptr<nsIPrefBranch> prefs = nsGetPrefBranch();
    if (prefs.empty())
        return;

    prefs->SetBoolPref((const char*)name.mbc_str(), value ? PR_TRUE : PR_FALSE);
}

