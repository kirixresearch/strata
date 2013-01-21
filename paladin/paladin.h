/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-25
 *
 */


#ifndef __PALADIN_PALADIN_H
#define __PALADIN_PALADIN_H


#ifdef _MSC_VER
#pragma comment(lib, "snmpapi")
#pragma comment(lib, "wsock32")
#endif


#include <string>


namespace paladin
{


typedef unsigned long featureid_t;

#ifdef _MSC_VER
typedef __int64 paladin_int64_t;
typedef __int64 sitecode_t;
typedef __int64 actcode_t;
#else
typedef long long paladin_int64_t;
typedef long long sitecode_t;
typedef long long actcode_t;
#endif


enum
{
    modeApp = 0,        // application global object (saves to disk)
    modeLocal = 1       // local testing object (doesn't save to disk)
};



enum
{
    errNone = 0,
    errAuthFailed = -1,     // computer did not have a valid license
    errClockModified = -2,  // computer's system clock was set back
    errClockUnsynced = -3,  // computer's local time is set wrong, and
                            // does not match the generated activation code
};


class Authentication
{

public:

    virtual std::string getCompanyRegKey() = 0;
    virtual std::string getAppTag() = 0;
    virtual std::string getEvalTag() = 0;

    virtual sitecode_t getSiteCode() = 0;
    virtual void setActivationCode(const actcode_t& act_code) = 0;

    virtual void deactivate() = 0;

    virtual int checkAuth() = 0;
    virtual int getDaysLeft() = 0;
    virtual bool isPerpetual() = 0;
    virtual featureid_t getFeatureId() = 0;

    virtual bool installLicense(int days) = 0;
    virtual bool installFingerprint() = 0;
    virtual bool getFingerprint() = 0;
};



Authentication* createAuthObject(const std::string& company_reg_key,
                                 const std::string& app_tag,
                                 const std::string& eval_tag,
                                 int mode = modeApp);


};


#include "util.h"



#endif



