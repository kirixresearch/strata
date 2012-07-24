/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-06-04
 *
 */


#ifndef __APP_INETAUTH_H
#define __APP_INETAUTH_H


namespace paladin
{
    class Authentication;
};


class InetAuth
{
public:

    enum
    {
        errorSuccess = 0,           // no error encountered
        errorFailure = 1,           // generic failure
        errorNetworkError = 2,      // network error (could not connect, server down, etc.)
        errorLoginInvalid = 3,      // login (username/password) was invalid
        errorProductMismatch = 4,   // license is for a different application
        errorLicenseExpired = 5,    // license has expired
        errorLicenseNotAvail = 6,   // license quota has been reached
        errorClockUnsynced = 7      // local clock does not have the correct time
    };


    static int authorize(paladin::Authentication* auth,
                         const wxString& version,
                         const wxString& license_login,
                         const wxString& license_password);

    static int deauthorize(paladin::Authentication* auth,
                           const wxString& license_login,
                           const wxString& license_password);

private:

    static wxString getAuthServer();

    static int installLicense(paladin::Authentication* auth,
                              const wxString& license_xml);

};



#endif

