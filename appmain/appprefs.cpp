/*!
 *
 * Copyright (c) 2008-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2008-03-14
 *
 */


#include "appmain.h"
#include "appprefs.h"
#include "../kcanvas/kcanvas.h"
#include "../kcanvas/graphicsobj.h"
#include <wx/fontenum.h>


// utility functions

wxColor getDefaultGreenBarColor(const wxColor& color, int ialpha)
{

    // make sure our default greenbar color is not too light
    wxColor c = kcl::stepColor(color, ialpha);
    if (c == *wxWHITE)
        return wxColor(248,248,248);
    if (c.Red() <= 240 || c.Green() <= 240 || c.Blue() <= 240)
        return c;
    if (c.Red() <= 248 && c.Green() <= 248 && c.Blue() <= 248)
        return c;
    if ((255-c.Red()) + (255-c.Green()) + (255-c.Blue()) <= 20)
        return getDefaultGreenBarColor(color, ialpha-1);
    return c;
}

wxFont getDefaultWindowsFont()
{
    if (wxFontEnumerator::IsValidFacename(wxT("Tahoma")))
        return wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Tahoma"));
         else
        return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
}




// populates the default preferences object with default values

void initDefaultPreferences()
{
    // we don't have a default preferences object, bail out
    IAppPreferencesPtr prefs = g_app->getAppDefaultPreferences();
    if (prefs.isNull())
        return;
    
    // default preferences have already been added, bail out
    std::vector<AppPreference> pref_vec;
    prefs->getAll(pref_vec);
    if (pref_vec.size() > 0)
        return;
    
    
    // add default general preferences
    prefs->setString(wxT("general.location.home"),              APP_WEBLOCATION_HOME);
    prefs->setString(wxT("general.location.help"),              APP_WEBLOCATION_HELP);
    prefs->setString(wxT("general.location.resources"),         APP_WEBLOCATION_DEVELOPER);
    prefs->setString(wxT("general.location.support"),           APP_WEBLOCATION_SUPPORT);
    prefs->setString(wxT("general.location.contact_us"),        APP_WEBLOCATION_CONTACT);

    prefs->setBoolean(wxT("general.startup.open_last_project"), true);
    prefs->setString(wxT("general.startup.default_project"),    wxEmptyString);
    prefs->setLong(wxT("general.startup.default_action"),       prefStartupOpenProject);
    prefs->setLong(wxT("general.toolbar.style"),                prefToolbarLargeIcons);
    prefs->setBoolean(wxT("general.updater.check_for_updates"), true);
    
    
    // add default internet preferences
    prefs->setLong(wxT("internet.proxy.type"),       prefProxyDirect);
    prefs->setLong(wxT("internet.proxy.http_port"),  0);
    prefs->setLong(wxT("internet.proxy.ftp_port"),   0);
    prefs->setLong(wxT("internet.proxy.ssl_port"),   0);
    prefs->setLong(wxT("internet.proxy.socks_port"), 0);
    prefs->setString(wxT("internet.proxy.http"),     wxEmptyString);
    prefs->setString(wxT("internet.proxy.ftp"),      wxEmptyString);
    prefs->setString(wxT("internet.proxy.ssl"),      wxEmptyString);
    prefs->setString(wxT("internet.proxy.socks"),    wxEmptyString);
    prefs->setString(wxT("internet.default_web_browser"), "app");
    
    // add default privacy preferences
    prefs->setBoolean(wxT("privacy.popups.block"),        true);
    prefs->setBoolean(wxT("privacy.cookie.accept"),       true);
    prefs->setLong(wxT("privacy.cookie.lifetime_policy"), prefKeepCookiesUntilExpired);
    prefs->setLong(wxT("privacy.cookie.lifetime_days"),   90);
    
    // add default regional preferences
    prefs->setString(wxT("region.language"), "default");

    // add default grid preferences
    #ifdef WIN32
    const wxFont datasheet_font = getDefaultWindowsFont();
    #else
    const wxFont datasheet_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    #endif
    
    prefs->setBoolean(wxT("grid.horzlines_visible"), true);
    prefs->setBoolean(wxT("grid.vertlines_visible"), true);
    prefs->setBoolean(wxT("grid.greenbar_visible"),  false);
    prefs->setBoolean(wxT("grid.boolean_checkbox"),  false);
    prefs->setBoolean(wxT("grid.text_wrap"),         false);
    
    prefs->setLong(wxT("grid.row_height"),           20);
    prefs->setLong(wxT("grid.greenbar_interval"),    2);
    
    prefs->setColor(wxT("grid.background_color"),    *wxWHITE);
    prefs->setColor(wxT("grid.foreground_color"),    *wxBLACK);
    prefs->setColor(wxT("grid.line_color"),          kcl::stepColor(kcl::getBorderColor(), 130));
    prefs->setColor(wxT("grid.invalid_area_color"),  wxColor(128,128,128));
    prefs->setColor(wxT("grid.greenbar_color"),      getDefaultGreenBarColor(kcl::getHighlightColor(), 195));
    
    prefs->setString(wxT("grid.font.face_name"),     datasheet_font.GetFaceName());
    prefs->setLong(wxT("grid.font.point_size"),      datasheet_font.GetPointSize());
    prefs->setLong(wxT("grid.font.family"),          datasheet_font.GetFamily());
    prefs->setLong(wxT("grid.font.style"),           datasheet_font.GetStyle());
    prefs->setLong(wxT("grid.font.weight"),          datasheet_font.GetWeight());
    prefs->setLong(wxT("grid.font.encoding"),        (long)datasheet_font.GetDefaultEncoding());
    prefs->setBoolean(wxT("grid.font.underline"),    datasheet_font.GetUnderlined());
    
    
    // add default report preferences
    
    // create a suitable default font to ensure we have
    // one that will work on a given system
    kcanvas::Font default_font;

    #ifdef WIN32
        default_font.setFaceName(wxT("Arial"));
    #else 
        default_font.setFaceName(wxT("Bitstream Vera Sans"));
    #endif

    default_font.setStyle(kcanvas::FONT_STYLE_NORMAL);
    default_font.setWeight(kcanvas::FONT_WEIGHT_NORMAL);
    default_font.setUnderscore(kcanvas::FONT_UNDERSCORE_NORMAL);
    default_font.setSize(10);    
    
    // (these values are in hundreths of an inch)
    prefs->setLong(wxT("report.page.width"),        850);
    prefs->setLong(wxT("report.page.height"),       1100);
    prefs->setLong(wxT("report.margin.left"),       100);
    prefs->setLong(wxT("report.margin.right"),      100);
    prefs->setLong(wxT("report.margin.top"),        100);
    prefs->setLong(wxT("report.margin.bottom"),     100);
    
    prefs->setString(wxT("report.font.facename"),   default_font.getFaceName());
    prefs->setLong(wxT("report.font.size"),         default_font.getSize());
    prefs->setString(wxT("report.font.style"),      default_font.getStyle());
    prefs->setString(wxT("report.font.weight"),     default_font.getWeight());
    prefs->setString(wxT("report.font.underscore"), default_font.getUnderscore());
    
    // add default script preferences
    prefs->setBoolean(wxT("script.show_syntax_highlighting"), true);
    prefs->setBoolean(wxT("script.show_line_numbers"),        true);
    prefs->setBoolean(wxT("script.show_whitespace"),          false);
    prefs->setBoolean(wxT("script.insert_spaces"),            true);
    prefs->setLong(wxT("script.insert_spaces_count"),         4);
    prefs->setLong(wxT("script.tab_size"),                    4);
}




// helper functions for getting preference default values

wxString getAppPrefsDefaultString(const wxString& pref_name)
{
    return g_app->getAppDefaultPreferences()->getString(pref_name, wxEmptyString);
}

wxColor getAppPrefsDefaultColor(const wxString& pref_name)
{
    return g_app->getAppDefaultPreferences()->getColor(pref_name, *wxBLACK);
}

bool getAppPrefsDefaultBoolean(const wxString& pref_name)
{
    return g_app->getAppDefaultPreferences()->getBoolean(pref_name, false);
}

long getAppPrefsDefaultLong(const wxString& pref_name)
{
    return g_app->getAppDefaultPreferences()->getLong(pref_name, 0);
}




// helper functions for getting preference values

wxString getAppPrefsString(const wxString& pref_name)
{
    wxString default_val = getAppPrefsDefaultString(pref_name);
    return g_app->getAppPreferences()->getString(pref_name, default_val);
}

wxColor getAppPrefsColor(const wxString& pref_name)
{
    wxColor default_val = getAppPrefsDefaultColor(pref_name);
    return g_app->getAppPreferences()->getColor(pref_name, default_val);
}

bool getAppPrefsBoolean(const wxString& pref_name)
{
    bool default_val = getAppPrefsDefaultBoolean(pref_name);
    return g_app->getAppPreferences()->getBoolean(pref_name, default_val);
}

long getAppPrefsLong(const wxString& pref_name)
{
    long default_val = getAppPrefsDefaultLong(pref_name);
    return g_app->getAppPreferences()->getLong(pref_name, default_val);
}




// helper functions for getting custom color preferences

void getAppPrefsCustomColors(std::vector<wxColor>& custom_colors)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    wxString pref_name = wxT("colorpanel.custom_color0");

    custom_colors.clear();
    for (int i = 0; i < 16; ++i)
    {
        pref_name.Printf(wxT("colorpanel.custom_color%d"), i);
        custom_colors.push_back(prefs->getColor(pref_name, *wxWHITE));
    }
}

void setAppPrefsCustomColors(std::vector<wxColor> custom_colors)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    wxString pref_name;
    int i = 0;

    // save the custom colors to the registry
    std::vector<wxColor>::iterator it;
    for (it = custom_colors.begin(); it != custom_colors.end(); ++it)
    {
        pref_name.Printf(wxT("colorpanel.custom_color%d"), i);
        prefs->setColor(pref_name, *it);
        i++;
    }

    prefs->flush();
}




// helper functions for getting find preferences

void getAppPrefsFindMatchCase(bool* match_case)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    wxString pref_name = wxT("find.match_case");
    *match_case = prefs->getBoolean(pref_name, false);
}

void getAppPrefsFindMatchWholeWord(bool* whole_word)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    wxString pref_name = wxT("find.whole_cells");
    *whole_word = prefs->getBoolean(pref_name, false);
}

void getAppPrefsFindInAllOpenDocuments(bool* all_open_documents)
{
    IAppPreferencesPtr prefs = g_app->getAppPreferences();
    wxString pref_name = wxT("find.all_open_documents");
    *all_open_documents = prefs->getBoolean(pref_name, false);
}



