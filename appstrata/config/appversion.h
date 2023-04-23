/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2003-01-11
 *
 */


#ifndef H_APP_APPVERSION_H
#define H_APP_APPVERSION_H


#include "build_config.h"


// version information

#ifndef APP_VERSION_MAJOR
#define APP_VERSION_MAJOR           4    // between 00 and 99, inclusive
#define APP_VERSION_MINOR           5    // between  0 and 9, inclusive
#define APP_VERSION_SUBMINOR        4    // between  0 and 9, inclusive
#define APP_VERSION_BUILDSERIAL     0    // between 0000 and 9999, inclusive
#endif


// incrementing this number will give users a new chance 
// for a new 60-day temporary license
// Increment it like this: TMP1, TMP2 .. TMP9, TMP10, TMP11, etc.

#ifndef APP_PALADIN_TEMP_LICENSE_COUNTER
#define APP_PALADIN_TEMP_LICENSE_COUNTER  "TMP18"
#endif


#define APPLICATION_NAME                wxT("Kirix Strata")
#define APP_COMPANY_NAME                wxT("Kirix Research")
#define APP_COPYRIGHT                   wxT("Copyright (c) 2002-2022, Kirix Research, LLC.  All rights reserved.")
#define APP_WEBLOCATION_HOME            wxT("https://www.kirix.com")
#define APP_WEBLOCATION_HELP            wxT("https://www.kirix.com/help/docs/")
#define APP_WEBLOCATION_STORE           wxT("https://store.kirix.com/store")
#define APP_WEBLOCATION_ACCOUNT         wxT("https://store.kirix.com/account/")
#define APP_WEBLOCATION_DEVELOPER       wxT("https://www.kirix.com/extensions/developer-resources.html")
#define APP_WEBLOCATION_SUPPORT         wxT("https://forums.kirix.com/")
#define APP_WEBLOCATION_FUNCTIONHELP    wxT("https://www.kirix.com/help/docs/index.html?%s.htm")
#define APP_WEBLOCATION_CONTACT         wxT("https://www.kirix.com/contact-us.html")
#define APP_WEBLOCATION_WELCOME1        wxT("https://www.kirix.com/strata/portal.html")
#define APP_WEBLOCATION_WELCOME2        wxT("https://www.kirix.com/strata/portal.html")
#define APP_WEBLOCATION_LICENSEEXPIRING wxT("https://www.kirix.com/strata/expiration")
#define APP_WEBLOCATION_WEBSITENAME     wxT("www.kirix.com")
#define APP_CONTACT_SUPPORTEMAIL        wxT("support@kirix.com")
#define APP_CONTACT_SALESEMAIL          wxT("sales@kirix.com")
#define APP_CONTACT_SUPPORTTELNO        wxT("630-563-2796")
#define APP_INETAUTH_SERVER             wxT("http://www.kirix.com/kms/appauth_engine.php")
#define APP_INETAUTH_AUTHSERVERLIST     wxT("http://www.kirix.com/kms/appauth.xml")
#define APP_UPDATE_ENCRYPTIONKEY        wxT("Vj%82lk0@1zVnqG%aM3dF!7a")
#define APP_UPDATE_URL                  wxT("http://www.kirix.com/update/update.xml")
#define APP_WELCOME_PAGES               1
#define APP_TRIAL_LICENSE_DAYS          30
#define PALADIN_APP_TAG                 "KSTRATA"
#define PALADIN_ENABLED                 0


// configuration key name

#ifdef WIN32
#define APP_CONFIG_KEY     wxT("Strata")               // windows registry key name
#define APP_COMPANY_KEY    wxT("Kirix")
#else
#define APP_CONFIG_KEY     wxT("kirix-strata")         // mac/linux dot-folder
#define APP_COMPANY_KEY    wxT("Kirix")
#endif



// ---------------------- end of configuration portion ----------------------

// make sure that APP_CONSOLE is somehow defined
#if !defined(APP_CONSOLE)
#define APP_CONSOLE 0
#endif

#if !defined(APP_GUI)
#if APP_CONSOLE==0
#define APP_GUI 1
#else
#define APP_GUI 0
#endif
#endif




// version parts for the resource file (usually do not modify these)

#define APP_RCVER_PART1 APP_VERSION_MAJOR
#define APP_RCVER_PART2 APP_VERSION_MINOR
#define APP_RCVER_PART3 APP_VERSION_SUBMINOR
#define APP_RCVER_PART4 APP_VERSION_BUILDSERIAL

#endif
