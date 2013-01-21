/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2008-03-14
 *
 */


#ifndef __APP_APPPREFS_H
#define __APP_APPPREFS_H


// IMPORTANT NOTE: The following preference values are immutable.
//     This means that they never may be changed.  New preference
//     values must be appended as a new number.

enum StartupOption
{
    prefStartupNone = 0,
    prefStartupProjectMgr = 1,
    prefStartupOpenProject = 2
};

enum StandardToolbarStyle
{
    prefToolbarSmallIcons = 0,
    prefToolbarLargeIcons = 1,
    prefToolbarLargeIconsWithText = 2
};

enum ProxyConnectionType
{
    prefProxyDirect     = 0,
    prefProxyAutoDetect = 1,
    prefProxyManual     = 2
};

enum KeepCookiesOption
{
    prefKeepCookiesUntilExpired = 0,
    prefKeepCookiesUntilExit = 1
};


// populate the default preferences object with default values
void initDefaultPreferences();


// these functions will return the default value of a preference

wxString getAppPrefsDefaultString(const wxString& pref_name);
wxColor getAppPrefsDefaultColor(const wxString& pref_name);
bool getAppPrefsDefaultBoolean(const wxString& pref_name);
long getAppPrefsDefaultLong(const wxString& pref_name);


// these functions will return either the value of a preference or,
// if the preference doesn't exist, the default value for that preference

wxString getAppPrefsString(const wxString& pref_name);
wxColor getAppPrefsColor(const wxString& pref_name);
bool getAppPrefsBoolean(const wxString& pref_name);
long getAppPrefsLong(const wxString& pref_name);


// helper functions for getting custom colors from the preferences

void getAppPrefsCustomColors(std::vector<wxColor>& custom_colors);
void setAppPrefsCustomColors(std::vector<wxColor> custom_colors);


// helper functions for getting find info from the preferences

void getAppPrefsFindMatchCase(bool* match_case);
void getAppPrefsFindMatchWholeWord(bool* whole_word);
void getAppPrefsFindInAllOpenDocuments(bool* all_open_documents);


#endif  // __APP_APPPREFS_H


