/*!
 *
 * Copyright (c) 2003-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Security Library
 * Author:   Benjamin I. Williams
 * Created:  2003-04-25
 *
 */


#include <kl/klib.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include "paladin.h"
#include "crypt.h"
#include "crc.h"
#include "util.h"


#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <comdef.h>
#include <tchar.h>
#endif


// these are defined here because the old
// VC6 SDK doesn't have them
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x1c
#endif


#ifndef CSIDL_PROFILE
#define CSIDL_PROFILE 0x28
#endif


namespace paladin
{


sitecode_t getHostId();

// our "generic" key (don't change)
static unsigned char generic_key[] = { 0x42, 0x49, 0x98, 0xba, 0x22, 0x34, 0x32, 0x8e };

// our "fingerprint" key (don't change)
static unsigned char fingerprint_key[] = { 0x22, 0xc9, 0x98, 0xb8, 0x32, 0x54, 0x22, 0x9e };



// -- activation code timestamp --

//  16-bit crc of the site code
//  16-bit Expiration date (Julian day - 1/1/2000)
//  16-bit AuthCode creation date (Julian day - 1/1/2000)
//  16-bit Feature id
//  xxxx xxxx xxxx xxxx


class AuthImpl : public Authentication
{
public:

    AuthImpl(const std::string& company_reg_key,
             const std::string& app_tag,
             const std::string& eval_tag,
             int mode);

    virtual ~AuthImpl() { }

    int checkAuth();
    int getDaysLeft();
    bool isPerpetual();
    featureid_t getFeatureId();

    sitecode_t getSiteCode();
    void setActivationCode(const actcode_t& act_code);

    void deactivate();

    std::string getCompanyRegKey();
    std::string getAppTag();
    std::string getEvalTag();

    bool installLicense(int days);
    bool installFingerprint();
    bool getFingerprint();

private:

    void generateFingerprints();
    
    bool loadAuthInfo();
    bool saveAuthInfo();
    void updateTimeStamp();
    
    int getSiteCodeSeed();
    void saveSiteCodeSeed(int seed);
    int loadSiteCodeSeed();

private:

    char m_company_regkey[255];
    char m_app_tag[255];
    char m_eval_tag[255];
    char m_fingerprint1[255];
    char m_fingerprint2[255];
    char m_fingerprint3[255];
    unsigned long m_eval_tag_crc;
    actcode_t m_act_code;
    featureid_t m_feature_id;
    paladin_int64_t m_time_stamp;
    int m_days_left;
    int m_mode;
    bool m_perpetual;
};



Authentication* createAuthObject(const std::string& company_reg_key,
                                 const std::string& app_tag,
                                 const std::string& eval_tag,
                                 int mode)
{
    AuthImpl* auth = new AuthImpl(company_reg_key, app_tag, eval_tag, mode);
    return static_cast<Authentication*>(auth);
}




#ifdef WIN32

const int regMachine = 0;
const int regUser = 1;
const int regClasses = 2;

bool writeRegKey(int scope, LPCTSTR path, LPCTSTR val)
{
    LONG result;
    HKEY hkey;
    HKEY root;

    switch (scope)
    {
        case regMachine: root = HKEY_LOCAL_MACHINE; break;
        case regUser:    root = HKEY_CURRENT_USER; break;
        case regClasses: root = HKEY_CLASSES_ROOT; break;
        default:
            return false;
    }

    result = RegCreateKeyEx(root, path, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hkey, NULL);

    if (result != ERROR_SUCCESS)
        return false;

    DWORD set_size = (DWORD)(_tcslen(val)+1);
    set_size *= (DWORD)(sizeof(TCHAR));

    result = RegSetValueEx(hkey, NULL, 0, REG_SZ, (const BYTE*)val, set_size);

    RegCloseKey(hkey);

    if (result != ERROR_SUCCESS)
        return false;

    return true;
}

bool getSpecialFolderPath(int folder_id, TCHAR* path)
{
    LPMALLOC pIMalloc = NULL;
    if (S_OK != SHGetMalloc(&pIMalloc))
        return false;

    LPITEMIDLIST pidl = NULL;
    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl)))
    {
        pIMalloc->Release();
        return false;
    }

    if (!pidl)
    {
        pIMalloc->Release();
        return false;
    }

    SHGetPathFromIDList(pidl, path);

    pIMalloc->Free(pidl);
    pIMalloc->Release();
    return true;
}

bool readRegKey(int scope, LPCTSTR path, LPTSTR val)
{
    LONG result;
    HKEY hkey;
    DWORD dtype;
    DWORD dlen;
    HKEY root;
    
    switch (scope)
    {
        case regMachine: root = HKEY_LOCAL_MACHINE; break;
        case regUser:    root = HKEY_CURRENT_USER; break;
        case regClasses: root = HKEY_CLASSES_ROOT; break;
        default:
            return false;
    }

    // load auth code

    result = RegCreateKeyEx(root, path, 0, NULL, REG_OPTION_NON_VOLATILE,
                   KEY_ALL_ACCESS, NULL, &hkey, NULL);

    if (result != ERROR_SUCCESS)
        return false;

    dtype = REG_SZ;
    dlen = 254;
    result = RegQueryValueEx(hkey, NULL, 0, &dtype, (LPBYTE)val, &dlen);
 
    RegCloseKey(hkey);

    if (result != ERROR_SUCCESS)
        return false;

    return true;
}

static bool writeReg(LPCTSTR path, LPCTSTR val)
{
    /*
    // for Terminal Server and Citrix environments, do not
    // attempt to write keys at the machine scope; this
    // will enforce per-user licensing on the above system types

    if (GetSystemMetrics(0x1000)) // SM_REMOTESESSION
    {
        return writeRegKey(regUser, path, val);
    }
    */
    
    // for normal windows systems, first try to write
    // the key to the machine root, then the user root

    if (!writeRegKey(regMachine, path, val))
    {
        return writeRegKey(regUser, path, val);
    }

    return true;
}

static bool readReg(LPCTSTR path, LPTSTR val)
{
/*
    // for Terminal Server and Citrix environments, do not
    // attempt to write keys at the machine scope; this
    // will enforce per-user licensing on the above system types

    if (GetSystemMetrics(0x1000)) // SM_REMOTESESSION
    {
        return readRegKey(regUser, path, val);
    }
*/
    // for normal windows systems, first try to read
    // the key from the machine root, then the user root

    if (!readRegKey(regMachine, path, val))
    {
        return readRegKey(regUser, path, val);
    }

    return true;
}


bool getRegKeyExist(int scope, LPCTSTR path, LPTSTR val)
{
    HKEY hkey;
    HKEY root;

    switch (scope)
    {
        case regMachine: root = HKEY_LOCAL_MACHINE; break;
        case regUser:    root = HKEY_CURRENT_USER; break;
        case regClasses: root = HKEY_CLASSES_ROOT; break;
        default:
            return false;
    }

    if (ERROR_SUCCESS != RegOpenKeyEx(root, path, 0, KEY_READ, &hkey))
    {
        return false;
    }

    RegCloseKey(hkey);

    return true;
}


#endif


bool writeFileMarker(const std::wstring& filename,
                     bool hidden,
                     bool set_old_time,
                     paladin_int64_t data = 0)
{
    remove(kl::tostring(filename).c_str());
    FILE* file = fopen(kl::tostring(filename).c_str(), "w+b");

    if (!file)
    {
        return false;
    }


    unsigned char* bytes = new unsigned char[4096];
    int size = ((rand()+1) % 8) * 224;
    int i;

    // fill it with garbage

    for (i = 0; i < size; ++i)
        bytes[i] = rand();

    // add a data mark
    int64unpack(bytes, data);


    fwrite(bytes, size, 1, file);
    fclose(file);

    delete[] bytes;

#ifdef WIN32

    if (set_old_time)
    {
        HANDLE h = CreateFile(kl::tstr(filename).c_str(),
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_ARCHIVE,
                              NULL);
        if (h != INVALID_HANDLE_VALUE)
        {
            FILETIME ft;
            if (GetFileTime(h, NULL, NULL, &ft))
            {
                LARGE_INTEGER li;
                memcpy(&li, &ft, sizeof(LARGE_INTEGER));
                li.QuadPart -= 39312000000000;     // subract off an arbitrary time
                memcpy(&ft, &li, sizeof(LARGE_INTEGER));
                SetFileTime(h, &ft, &ft, &ft);
            }

            CloseHandle(h);
        }
    }

    if (hidden)
    {
        SetFileAttributes(kl::tstr(filename).c_str(), FILE_ATTRIBUTE_HIDDEN);
    }

#endif

    return true;
}




// -- AuthImpl class implementation --


AuthImpl::AuthImpl(const std::string& company_reg_key,
                   const std::string& app_tag,
                   const std::string& eval_tag, int mode)
{
    m_days_left = 0;
    m_feature_id = 0;
    m_act_code = 0;
    m_time_stamp = 0;
    m_eval_tag_crc = 0;
    m_mode = mode;
    m_perpetual = false;
    
    strcpy(m_company_regkey, company_reg_key.c_str());
    strcpy(m_app_tag, app_tag.c_str());
    strcpy(m_eval_tag, eval_tag.c_str());
    
    generateFingerprints();
}

void AuthImpl::generateFingerprints()
{
    paladin_int64_t ik;
    
    // first fingerprint string will be the crc32 of the eval tag
    m_eval_tag_crc = crc32((unsigned char*)m_eval_tag, (int)strlen(m_eval_tag));
    snprintf(m_fingerprint1, sizeof(m_fingerprint1) - 1, "%08X", (unsigned int)(m_eval_tag_crc & 0xffffffff));

    // second fingerprint string will be an int64 suitably randomized
    // by an encryption key
    ik = m_eval_tag_crc;
    int64crypt(ik, generic_key, true);
    getStringFromCode(ik, m_fingerprint2, sizeof(m_fingerprint2) - 1, false);
    
    // second fingerprint string will be the same as the second
    // fingerprint, simply created with a different key
    ik = m_eval_tag_crc;
    int64crypt(ik, fingerprint_key, true);
    getStringFromCode(ik, m_fingerprint3, sizeof(m_fingerprint3) - 1, false);
}

bool AuthImpl::loadAuthInfo()
{
    static const char* hexchars = "0123456789ABCDEF";

    if (m_mode == modeLocal)
        return false;

    m_act_code = 0;
    m_time_stamp = 0;


    std::string str_code;

#ifdef WIN32
    // read info from registry

    TCHAR path[255];
    TCHAR temps[255];

    _sntprintf(path, 255, _T("SOFTWARE\\%s\\LicenseInfo\\%s\\AuthCode"),
                    (TCHAR*)kl::tstr(m_company_regkey),
                    (TCHAR*)kl::tstr(m_app_tag));

    if (!readReg(path, temps))
    {
        return false;
    }

    str_code = kl::tostring(temps);

#else

    std::string path = getenv("HOME");
    path += "/.paladin";

    if (!xf_get_directory_exist(kl::towstring(path)))
    {
        xf_mkdir(kl::towstring(path));
    }
    
    std::string filename = path;
    filename += "/";
    filename += m_app_tag;
    filename += ".i1";

    FILE* f = fopen(filename.c_str(), "rt");
    if (!f)
    {
        return false;
    }

    char buf[255];
    buf[0] = 0;
    char* tempp = fgets(buf, 254, f);
    str_code = buf;
    fclose(f);

#endif


    // decode our bytes

    unsigned char bytes[16];
    bool hi = true;
    const char* p = str_code.c_str();
    int off = 0;
    while (*p)
    {
        char c = toupper(*p);
        p++;

        if (isspace(c) || c == '-' || c == '.')
            continue;

        const char* pos = strchr(hexchars, c);
        if (!pos)
            return false;
        unsigned char v = (unsigned char)(pos-hexchars);

        if (hi)
        {
            bytes[off] = v << 4;
        }
         else
        {
            bytes[off] |= v;
            off++;
            if (off >= 16)
                break;
        }

        hi = !hi;
    }

    // decrypt bytes
    Des d;
    d.setKey(generic_key);
    d.crypt(bytes, 8, false);
    d.crypt(bytes+8, 8, false);

    unsigned char act_code_bytes[8];
    unsigned char time_stamp_bytes[8];

    time_stamp_bytes[0] =  bytes[0];
    act_code_bytes[0] =    bytes[1];
    time_stamp_bytes[2] =  bytes[2];
    act_code_bytes[2] =    bytes[3];
    time_stamp_bytes[4] =  bytes[4];
    act_code_bytes[4] =    bytes[5];
    time_stamp_bytes[6] =  bytes[6];
    act_code_bytes[6] =    bytes[7];
    time_stamp_bytes[1] =  bytes[8];
    act_code_bytes[1] =    bytes[9];
    time_stamp_bytes[3] =  bytes[10];
    act_code_bytes[3] =    bytes[11];
    time_stamp_bytes[5] =  bytes[12];
    act_code_bytes[5] =    bytes[13];
    time_stamp_bytes[7] =  bytes[14];
    act_code_bytes[7] =    bytes[15];

    int64pack(m_act_code, act_code_bytes);
    int64pack(m_time_stamp, time_stamp_bytes);

    return true;
}


bool AuthImpl::saveAuthInfo()
{
    if (m_mode == modeLocal)
        return false;

    updateTimeStamp();

    unsigned char act_code_bytes[8];
    unsigned char time_stamp_bytes[8];
    unsigned char bytes[16];
    int64unpack(act_code_bytes, m_act_code);
    int64unpack(time_stamp_bytes, m_time_stamp);
    bytes[0] = time_stamp_bytes[0];
    bytes[1] =   act_code_bytes[0];
    bytes[2] = time_stamp_bytes[2];
    bytes[3] =   act_code_bytes[2];
    bytes[4] = time_stamp_bytes[4];
    bytes[5] =   act_code_bytes[4];
    bytes[6] = time_stamp_bytes[6];
    bytes[7] =   act_code_bytes[6];
    bytes[8] = time_stamp_bytes[1];
    bytes[9] =   act_code_bytes[1];
    bytes[10] = time_stamp_bytes[3];
    bytes[11] =   act_code_bytes[3];
    bytes[12] = time_stamp_bytes[5];
    bytes[13] =   act_code_bytes[5];
    bytes[14] = time_stamp_bytes[7];
    bytes[15] =   act_code_bytes[7];

    Des d;
    d.setKey(generic_key);
    d.crypt(bytes, 8, true);
    d.crypt(bytes+8, 8, true);

    // save auth code

#ifdef WIN32

    TCHAR path[255];
    TCHAR temps[255];
    
    _sntprintf(path, 255, _T("SOFTWARE\\%s\\LicenseInfo\\%s\\AuthCode"),
                    (TCHAR*)kl::tstr(m_company_regkey),
                    (TCHAR*)kl::tstr(m_app_tag));

    _sntprintf(temps, 255, _T("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X"),
                    bytes[0], bytes[1], bytes[2], bytes[3],
                    bytes[4], bytes[5], bytes[6], bytes[7],
                    bytes[8], bytes[9], bytes[10], bytes[11],
                    bytes[12], bytes[13], bytes[14], bytes[15]);

    if (!writeReg(path, temps))
    {
        return false;
    }

    return true;

#else

    std::string path = getenv("HOME");
    path += "/.paladin";

    if (!xf_get_directory_exist(kl::towstring(path)))
    {
        xf_mkdir(kl::towstring(path));
    }
    
    std::string filename = path;
    filename += "/";
    filename += m_app_tag;
    filename += ".i1";

    FILE* f = fopen(filename.c_str(), "wt");
    if (f)
    {
        char temps[255];
        snprintf(temps, sizeof(temps)-1, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11],
        bytes[12], bytes[13], bytes[14], bytes[15]);
        fputs(temps, f);
        fclose(f);
    }

#endif

    return false;
}



void AuthImpl::updateTimeStamp()
{
#ifdef WIN32
    // this corrects the C runtime's timezone settings
    // in case the user updated the computer's timezone
    // settings
    _tzset();
#endif

    sitecode_t host_id = ::paladin::getHostId();
    unsigned int cur_time = (unsigned int)time(NULL);

    unsigned char host_id_bytes[8];
    int64unpack(host_id_bytes, host_id);

    unsigned char timestamp_bytes[8];
    timestamp_bytes[0] = (cur_time & 0xff);
    timestamp_bytes[1] = (cur_time >> 8) & 0xff;
    timestamp_bytes[2] = (cur_time >> 16) & 0xff;
    timestamp_bytes[3] = (cur_time >> 24) & 0xff;
    memcpy(timestamp_bytes+4, timestamp_bytes, 4);

    unsigned char timestamp_enc_key[8];
    memcpy(timestamp_enc_key, host_id_bytes, 8);
    timestamp_enc_key[6] = 0xfe;
    timestamp_enc_key[7] = 0x34;

    Des d;
    d.setKey(timestamp_enc_key);
    d.crypt(timestamp_bytes, 8, true);

    int64pack(m_time_stamp, timestamp_bytes);
}


int AuthImpl::checkAuth()
{
    loadAuthInfo();

    m_days_left = 0;
    m_feature_id = 0;

#ifdef WIN32
    // this corrects the C runtime's timezone settings
    // in case the user updated the computer's timezone
    // settings
    _tzset();
#endif

    time_t curtime;
    time(&curtime);
    struct tm *curtm;
    curtm = localtime(&curtime);
    unsigned int today_julian = dateToJulian(curtm->tm_year + 1900, curtm->tm_mon + 1, curtm->tm_mday);

    Des d;
    sitecode_t host_id = ::paladin::getHostId();

    unsigned char host_id_bytes[8];
    int64unpack(host_id_bytes, host_id);

    unsigned char act_code_bytes[8];
    int64unpack(act_code_bytes, m_act_code);

    // create master key from application tag and our finger-print

    unsigned char master_key[8];
    unsigned char enc_key[8];
    unsigned long app_tag_crc = crc32((unsigned char*)m_app_tag, (int)strlen(m_app_tag));

    master_key[0] = (unsigned char)((app_tag_crc) & 0xff);
    master_key[1] = (unsigned char)((app_tag_crc >> 8) & 0xff);
    master_key[2] = (unsigned char)((app_tag_crc >> 16) & 0xff);
    master_key[3] = (unsigned char)((app_tag_crc >> 24) & 0xff);
    master_key[4] = 0x8a;
    master_key[5] = 0x4b;
    master_key[6] = 0x22;
    master_key[7] = 0x1f;

    memcpy(enc_key, host_id_bytes, 8);
    d.setKey(master_key);
    d.crypt(enc_key, 8, true);

    // decrypt the activation code
    d.setKey(enc_key);
    d.crypt(act_code_bytes, 8, false);


    // check it

    unsigned short host_id_crc;
    unsigned int exp_date, actcode_create_date;

    host_id_crc = act_code_bytes[1];
    host_id_crc <<= 8;
    host_id_crc |= act_code_bytes[0];

    unsigned short real_host_id_crc = crc16(host_id_bytes, 8);

    if (host_id_crc != real_host_id_crc)
        return errAuthFailed;

    exp_date = act_code_bytes[3];
    exp_date <<= 8;
    exp_date |= act_code_bytes[2];
    
    // an expiration date of 0xffaa means that this license is a 'perpetual',
    // i.e. it doesn't expire
    
    bool perpetual = false;
    if (exp_date == 0xffaa)
        perpetual = true;
    
    exp_date += dateToJulian(2000, 1, 1);

    actcode_create_date = act_code_bytes[5];
    actcode_create_date <<= 8;
    actcode_create_date |= act_code_bytes[4];
    actcode_create_date += dateToJulian(2000, 1, 1);
    

    if (!perpetual)
    {
        if (exp_date < today_julian)
        {
            saveAuthInfo();
            return errAuthFailed;
        }
            
        if (exp_date > today_julian)
        {
            if (exp_date - today_julian > 7400)
            {
                // don't accept licenses with expiration dates past 20 years
                // from now -- this limits the number of valid codes.
                saveAuthInfo();
                return errAuthFailed;
            }
        }
    
        if (today_julian+1 < actcode_create_date)
            return errClockUnsynced;
    
    
        // authorization checks out, now check clock. But allow a tolerance
        // of 2 hrs, in case they are adjusting their computer clock
    
        if (m_mode == modeApp)
        {
            unsigned char timestamp_bytes[8];
            int64unpack(timestamp_bytes, m_time_stamp);
    
            unsigned char timestamp_enc_key[8];
            memcpy(timestamp_enc_key, host_id_bytes, 8);
            timestamp_enc_key[6] = 0xfe;
            timestamp_enc_key[7] = 0x34;
    
            d.setKey(timestamp_enc_key);
            d.crypt(timestamp_bytes, 8, false);
    
            if (0 != memcmp(timestamp_bytes, timestamp_bytes+4, 4))
            {
                // clock value is not valid
                return errAuthFailed;
            }
    
            unsigned int last_clock;
            
            last_clock = ((unsigned int)timestamp_bytes[0]) |
                         (((unsigned int)timestamp_bytes[1]) << 8) |
                         (((unsigned int)timestamp_bytes[2]) << 16) |
                         (((unsigned int)timestamp_bytes[3]) << 24);
    
            if (last_clock-3600 > curtime)
            {
                return errClockModified;
            }
    
            saveAuthInfo();
        }
        
    } // if (!perpetual)

    if (perpetual)
    {
        m_perpetual = true;
    }
     else
    {
        m_perpetual = false;
    }
    
    m_days_left = exp_date - today_julian;

    m_feature_id = 0;
    unsigned long l;
    l = act_code_bytes[7];
    m_feature_id |= (l << 8);
    l = act_code_bytes[6];
    m_feature_id |= (l);

    return errNone;
}

int AuthImpl::getDaysLeft()
{
    // have to run isAuthorized to update the m_days_left variable
    if (checkAuth() != errNone)
        return 0;

    return m_days_left;
}

bool AuthImpl::isPerpetual()
{
    return m_perpetual;
}

featureid_t AuthImpl::getFeatureId()
{
    return m_feature_id;
}

std::string AuthImpl::getCompanyRegKey()
{
    return m_company_regkey;
}

std::string AuthImpl::getAppTag()
{
    return m_app_tag;
}

std::string AuthImpl::getEvalTag()
{
    return m_eval_tag;
}


sitecode_t AuthImpl::getSiteCode()
{
    // the unencrypted 64-bit site code will look like this:
    // VVSS HHHH HHHH HHHH
    //
    // where:  V = verify nibble (rest of site code mod 255)
    //         S = 8-bit site code seed
    //         H = 48-bit host id
   
    // getHostId() returns a 48-bit computer identifier.
    // This will be the basis of our site code
    sitecode_t site_code = ::paladin::getHostId();
    
    
    // we will add a random 8-bits to that to randomize it so that,
    // when encrypted, it will be different every time --
    sitecode_t r = getSiteCodeSeed();
    r &= 0xff; // make sure it's 8 bits
    
    // add the site code seed to the left of the site code
    site_code |= (r << 48);
    
    // calculate and add check byte to the very left-most byte
    // inside the side code.  This will help us
    // tell what a good site code is from a bad one
#ifdef _MSC_VER
    site_code &= 0x00ffffffffffffff;
#else
    site_code &= 0x00ffffffffffffffLL;
#endif
    
    sitecode_t check_byte = (site_code % 0xff);
    check_byte <<= 56;
    site_code |= check_byte;
    
    // mix it all up by encrypting it
    int64crypt(site_code, generic_key, true);

    return site_code;
}


/*

bool validateSiteCode(const wxString& val_site_code)
{
    paladin::sitecode_t site_code = getCodeFromString(val_site_code);

    // decrypt the side code
    paladin::int64crypt(site_code, generic_key, false);

    unsigned char check_byte = (unsigned char)(site_code >> 56);
    
    site_code &= 0x00ffffffffffffff;
    if ((site_code % 0xff) == check_byte)
        return true;
        
    return false;
}

*/

// generates an 8-bit seed code used for
// randomizing site codes
static int generateSeedCode()
{
    int seed = (int)(time(NULL) & 0xffff);
    seed += clock();
    seed += rand();
    seed &= 0xff;

    return seed;
}


void AuthImpl::setActivationCode(const actcode_t& act_code)
{
    // decrypt the activation code using the site code as the key

    sitecode_t site_code = getSiteCode();

    unsigned char site_code_bytes[8];
    int64unpack(site_code_bytes, site_code);

    m_act_code = act_code;
    int64crypt(m_act_code, site_code_bytes, false);

    saveAuthInfo();


    if (m_mode == modeApp && checkAuth() == errNone)
    {
        if (!getFingerprint())
        {
            installFingerprint();
        }

        // because the authorization was ok, we can generate
        // a new site code seed for the next time
        
        int seed = generateSeedCode();
        saveSiteCodeSeed(seed);
    }
}


void AuthImpl::deactivate()
{
    setActivationCode(0);
}



bool AuthImpl::installLicense(int temp_license_days)
{
    if (getFingerprint())
        return false;

    if (m_mode != modeApp)
        return false;

    if (!installFingerprint())
        return false;

    // prepare site_code and host_id code byte arrays

    sitecode_t site_code = getSiteCode();
    unsigned char site_code_bytes[8];
    paladin::int64unpack(site_code_bytes, site_code);

    unsigned char host_id_bytes[8];
    paladin::sitecode_t host_id = ::paladin::getHostId();
    paladin::int64unpack(host_id_bytes, host_id);


    // calculate activation code
    time_t t = time(NULL);
    struct tm tm;
    
    localtime_r(&t, &tm);

    unsigned char actcode[8];
    unsigned int crc = crc16(host_id_bytes, 8);
    unsigned int expire_julian = dateToJulian(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday) - dateToJulian(2000, 1, 1) + temp_license_days;
    unsigned int createdt_julian = dateToJulian(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday) - dateToJulian(2000, 1, 1);
    unsigned int feature_id = 0;

    actcode[0] = crc & 0xff;
    actcode[1] = (crc >> 8) & 0xff;
    actcode[2] = (expire_julian) & 0xff;
    actcode[3] = (expire_julian >> 8) & 0xff;
    actcode[4] = (createdt_julian) & 0xff;
    actcode[5] = (createdt_julian >> 8) & 0xff;
    actcode[6] = (feature_id) & 0xff;
    actcode[7] = (feature_id >> 8) & 0xff;


    // encrypt site code with the master encryption key

    Des d;

    unsigned char master_key[8];

    char buf[255];
    strcpy(buf, m_app_tag);
    unsigned long app_tag_crc = crc32((unsigned char*)buf, (int)strlen(buf));

    master_key[0] = (unsigned char)((app_tag_crc) & 0xff);
    master_key[1] = (unsigned char)((app_tag_crc >> 8) & 0xff);
    master_key[2] = (unsigned char)((app_tag_crc >> 16) & 0xff);
    master_key[3] = (unsigned char)((app_tag_crc >> 24) & 0xff);
    master_key[4] = 0x8a;
    master_key[5] = 0x4b;
    master_key[6] = 0x22;
    master_key[7] = 0x1f;


    unsigned char enc_key[8];
    memcpy(enc_key, host_id_bytes, 8);
    d.setKey(master_key);
    d.crypt(enc_key, 8, true);

    // encrypt activation code
    d.setKey(enc_key);
    d.crypt(actcode, 8, true);

    // encrypt activation with site key
    d.setKey(site_code_bytes);
    d.crypt(actcode, 8, true);


    // generate final activation code from buffer

    paladin::actcode_t final_actcode = 0;
    paladin::int64pack(final_actcode, actcode);

    setActivationCode(final_actcode);

    return true;
}






// -- fingerprint functions and methods --

#ifdef WIN32

// {BAA05053-28EB-4376-9D54-B0AF89C15A98}
static const GUID guid_fp1 = 
{ 0xbaa05053, 0x28eb, 0x4376, { 0x9d, 0x54, 0xb0, 0xaf, 0x89, 0xc1, 0x5a, 0x98 } };

// {88E0A525-57F4-44ce-844E-4954FB1AEAE1}
static const GUID guid_fp2 = 
{ 0x88e0a525, 0x57f4, 0x44ce, { 0x84, 0x4e, 0x49, 0x54, 0xfb, 0x1a, 0xea, 0xe1 } };

// {552866D6-09F2-4a3c-8573-F6745B7F217E}
static const GUID guid_fp3 = 
{ 0x552866d6, 0x9f2, 0x4a3c, { 0x85, 0x73, 0xf6, 0x74, 0x5b, 0x7f, 0x21, 0x7e } };

#endif


#ifdef WIN32
void buildCLSIDPath(LPCTSTR path1, const GUID& guid, TCHAR* path)
{
    // this builds a fake path which looks like
    // it shouldn't be removed.  It is really used
    // as a fingerprint to identify if a package
    // has been installed or not
    
    wchar_t guidstr[255];
    _tcscpy(path, path1);
    _tcscat(path, _T("\\"));
    (void)StringFromGUID2(guid, guidstr, 254);
    _tcscat(path, kl::tstr(guidstr));
    _tcscat(path, _T("\\Option"));
}


static void appendPathPart(LPTSTR path, LPCTSTR part)
{
    if (*path == 0)
    {
        _tcscpy(path, part);
        return;
    }

    if (*(path + _tcslen(path) - 1) != _T('\\'))
    {
        TCHAR s[2];
        s[0] = _T('\\');
        s[1] = 0;
        _tcscat(path, s);
    }
    
    while (*part == _T('\\'))
        part++;

    _tcscat(path, part);
}


#endif

bool AuthImpl::installFingerprint()
{
#ifdef WIN32

    if (getFingerprint())
        return true;

    TCHAR path[512];
    UUID guid;

    // write a fingerprint in
    // x:\Documents and Settings\username\Application Data
    if (getSpecialFolderPath(CSIDL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint1));
        writeFileMarker(kl::towstring(path), true, true);
    }
    
    // write a fingerprint in
    // x:\Documents and Settings\username\Local Settings\Application Data
    if (getSpecialFolderPath(CSIDL_LOCAL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint2));
        writeFileMarker(kl::towstring(path), true, true);
    }
    
    // write a fingerprint in
    // x:\Documents and Settings\username
    if (getSpecialFolderPath(CSIDL_PROFILE, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint3));
        writeFileMarker(kl::towstring(path), true, true);
    }



    // write registry fingerprints
    TCHAR base_path[255];
    _sntprintf(base_path, 255, _T("Software\\%s\\Security"),
                    (TCHAR*)kl::tstr(m_company_regkey));
                    
    memcpy(&guid, &guid_fp1, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(base_path, guid, path);
    writeRegKey(regUser, path, _T(""));

    memcpy(&guid, &guid_fp2, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    writeRegKey(regUser, path, _T(""));

    memcpy(&guid, &guid_fp3, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    writeRegKey(regUser, path, _T(""));

    return true;

#else

    std::wstring eval_tag;
    eval_tag = kl::towstring(m_eval_tag);
    kl::makeLower(eval_tag);
    char rot13_str[128];
    strcpy(rot13_str, kl::tstr(eval_tag));
    rot13(rot13_str);
    
    std::wstring path = kl::towstring(getenv("HOME"));
    path += L"/.";
    path += kl::towstring(rot13_str);

    writeFileMarker(path, true, true);
    
    return true;

#endif
}




bool AuthImpl::getFingerprint()
{
#ifdef WIN32

    TCHAR path[512];
    UUID guid;


    // write a fingerprint in
    // x:\Documents and Settings\username\Application Data
    if (getSpecialFolderPath(CSIDL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint1));
        if (xf_get_file_exist((wchar_t*)kl::tstr(path)))
            return true;
    }
    
    // write a fingerprint in
    // x:\Documents and Settings\username\Local Settings\Application Data
    if (getSpecialFolderPath(CSIDL_LOCAL_APPDATA, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint2));
        if (xf_get_file_exist((wchar_t*)kl::tstr(path)))
            return true;
    }
    
    // write a fingerprint in
    // x:\Documents and Settings\username
    if (getSpecialFolderPath(CSIDL_PROFILE, path))
    {
        appendPathPart(path, kl::tstr(m_fingerprint3));
        if (xf_get_file_exist((wchar_t*)kl::tstr(path)))
            return true;
    }


    TCHAR base_path[255];
    _sntprintf(base_path, 255, _T("Software\\%s\\Security"),
                    (TCHAR*)kl::tstr(m_company_regkey));

    memcpy(&guid, &guid_fp1, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(base_path, guid, path);
    if (getRegKeyExist(regUser, path, _T("")))
        return true;

    memcpy(&guid, &guid_fp2, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    if (getRegKeyExist(regUser, path, _T("")))
        return true;

    memcpy(&guid, &guid_fp3, sizeof(UUID));
    guid.Data2 = (unsigned short)((m_eval_tag_crc & 0xffff0000) >> 16);
    guid.Data3 = (unsigned short)(m_eval_tag_crc & 0x0000ffff);
    buildCLSIDPath(_T("Software\\Pldx\\Entries"), guid, path);
    if (getRegKeyExist(regUser, path, _T("")))
        return true;


#else

    std::wstring eval_tag;
    eval_tag = kl::towstring(m_eval_tag);
    kl::makeLower(eval_tag);
    char rot13_str[128];
    strcpy(rot13_str, kl::tstr(eval_tag));
    rot13(rot13_str);
    
    std::wstring path = kl::towstring(getenv("HOME"));
    path += L"/.";
    path += kl::towstring(rot13_str);
    
    if (xf_get_file_exist(path))
        return true;
    
    return false;
    
#endif

    return false;
}







// site code seed functions and methods

int AuthImpl::getSiteCodeSeed()
{
    int seed = loadSiteCodeSeed();

    if (seed == -1)
    {
        seed = generateSeedCode();
        saveSiteCodeSeed(seed);
    }

    return seed;
}

void AuthImpl::saveSiteCodeSeed(int seed)
{

#ifdef WIN32

    TCHAR path[512];
    TCHAR seed_str[256];

    _sntprintf(seed_str, 255, _T("%d"), seed);

    // write seed into registry

    _sntprintf(path, 511, _T("SOFTWARE\\%s\\LicenseInfo\\%s\\ExtraInfo1"),
                    (TCHAR*)kl::tstr(m_company_regkey),
                    (TCHAR*)kl::tstr(m_app_tag));

    if (!writeReg(path, seed_str))
    {
        return;
    }


    // also write seed into home directory

    if (GetEnvironmentVariable(_T("HOMEPATH"), path, 511))
    {
        TCHAR tag_crc[255];
        _sntprintf(tag_crc, 255, _T("%08X.INF"), crc32((unsigned char*)m_app_tag, (int)strlen(m_app_tag)));
        appendPathPart(path, tag_crc);

        writeFileMarker(kl::towstring(path), true, true, seed);
    }

#else
    
    std::string path = getenv("HOME");
    path += "/.paladin";

    char seed_str[255];
    snprintf(seed_str, sizeof(seed_str) - 1, "%d", seed);

    if (!xf_get_directory_exist(kl::towstring(path)))
    {
        xf_mkdir(kl::towstring(path));
    }
    
    std::string filename = path;
    filename += "/";
    filename += m_app_tag;
    filename += ".i2";

    FILE* f = fopen(filename.c_str(), "wt");
    if (f)
    {
        fputs(seed_str, f);
        fclose(f);
    }

#endif

}

int AuthImpl::loadSiteCodeSeed()
{

#ifdef WIN32

    TCHAR path[512];
    TCHAR seed_str[256];

    // load seed from registry

    _sntprintf(path, 255, _T("SOFTWARE\\%s\\LicenseInfo\\%s\\ExtraInfo1"),
                    (TCHAR*)kl::tstr(m_company_regkey),
                    (TCHAR*)kl::tstr(m_app_tag));

    if (!readReg(path, seed_str))
        return -1;

    int reg_seed = atoi(kl::tostring(seed_str).c_str());


    // also read the seed from the home directory

    TCHAR filename[512];
    if (GetEnvironmentVariable(_T("HOMEPATH"), filename, 511))
    {
        TCHAR crc[256];
        _sntprintf(crc, 255, _T("%08X.INF"), crc32((unsigned char*)m_app_tag, (int)strlen(m_app_tag)));
        crc[255] = 0;
        appendPathPart(filename, crc);
    }
     else
    {
        return reg_seed;
    }


    FILE* f = fopen(kl::tostring(filename).c_str(), "rb");
    if (!f)
    {
        return reg_seed;
    }

    unsigned char buf[8];
    if (8 != fread(buf, 1, 8, f))
    {
        fclose(f);
        return reg_seed;
    }

    paladin_int64_t i;
    int file_seed;
    int64pack(i, buf);
    file_seed = (int)i;

    fclose(f);

    if (file_seed != reg_seed)
    {
        // the seed from the registry doesn't match the seed
        // from our stored file, so generate a new seed

        int seed = generateSeedCode();
        saveSiteCodeSeed(seed);
        return seed;
    }

    return reg_seed;

#else

    std::string path = getenv("HOME");
    path += "/.paladin";

    if (!xf_get_directory_exist(kl::towstring(path)))
    {
        xf_mkdir(kl::towstring(path));
    }
    
    std::string filename = path;
    filename += "/";
    filename += m_app_tag;
    filename += ".i2";

    FILE* f = fopen(filename.c_str(), "rt");
    if (!f)
    {
        return -1;
    }


    char buf[255];
    char* tempp = fgets(buf, 254, f);
    int seed = atoi(buf);
    fclose(f);

    return seed;
#endif
}




};

