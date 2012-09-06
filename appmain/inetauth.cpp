/*!
 *
 * Copyright (c) 2004-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2004-06-04
 *
 */


#include "appmain.h"
#include <wx/url.h>
#include "inetauth.h"
#include "../paladin/paladin.h"
#include "../paladin/crc.h"
#include "apputil.h"


wxString InetAuth::getAuthServer()
{
    wxString server = APP_INETAUTH_SERVER;

    // first, get the list of authorization servers from the main authorization server
    wxString auth_servers_xml = getWebFile(APP_INETAUTH_AUTHSERVERLIST);
        
    if (!auth_servers_xml.IsEmpty())
    {
        kl::xmlnode root;
        if (root.parse(towstr(auth_servers_xml)))
        {
            kl::xmlnode& server_node = root.getChild(L"server");
            if (!server_node.isEmpty())
            {
                kl::xmlnode& location_node = server_node.getChild(L"location");
                if (!location_node.isEmpty())
                {
                    if (location_node.getNodeValue().length() > 0)
                    {
                        server = towx(location_node.getNodeValue());
                    }
                }
            }
        }
    }

    return server;
}



int InetAuth::installLicense(paladin::Authentication* global_auth,
                             const wxString& license)
{
    kl::xmlnode root;
    if (!root.parse(towstr(license)))
        return errorFailure;

    kl::xmlnode& apptag_node = root.getChild(L"app_tag");
    kl::xmlnode& actcode_node = root.getChild(L"act_code");
    kl::xmlnode& errorcode_node = root.getChild(L"error_code");

    std::wstring error_code = errorcode_node.getNodeValue();

    if (error_code == L"invalid_login")
        return errorLoginInvalid;

    if (error_code == L"product_mismatch")
        return errorProductMismatch;

    if (error_code == L"license_notavail")
        return errorLicenseNotAvail;

    if (0 != strcasecmp(kl::tostring(apptag_node.getNodeValue()).c_str(),
                        global_auth->getAppTag().c_str()))
    {
        return errorProductMismatch;
    }

    if (error_code != L"success")
    {
        return errorFailure;
    }

    int err = errorFailure;

    paladin::actcode_t act_code;
    act_code = paladin::getCodeFromString(towx(actcode_node.getNodeValue()).mbc_str());

    if (act_code == 0)
        return errorFailure;

    paladin::Authentication* auth;

    auth = paladin::createAuthObject(global_auth->getCompanyRegKey(),
                                     global_auth->getAppTag(),
                                     global_auth->getEvalTag(),
                                     paladin::modeLocal);

    auth->setActivationCode(act_code);

    int result = auth->checkAuth();
    
    if (result == paladin::errNone)
    {
        global_auth->setActivationCode(act_code);

        if (global_auth->checkAuth() == paladin::errNone)
            err = errorSuccess;
    }
     else if (result == paladin::errClockUnsynced)
    {
        return errorClockUnsynced;
    }
     else
    {
        return errorFailure;
    }

    delete auth;

    return err;
}

static void addGetParam(wxString& dest,
                        const wxString& param,
                        const wxString& value)
{
    if (dest.Length() > 0)
    {
        if (dest.Last() != wxT('?'))
            dest += wxT("&");
    }

    dest += param;
    dest += wxT("=");

    const wxChar* p;
    p = value.c_str();

    while (*p)
    {
        if (*p == wxT(' '))
        {
            dest += wxT("+");
        }
         else if (*p == wxT('+'))
        {
            dest += wxT("%2B");
        }
         else if (*p == wxT('%'))
        {
            dest += wxT("%25");
        }
         else
        {
            dest += *p;
        }

        ++p;
    }
}

#ifdef WIN32

#include <lm.h>
#pragma comment(lib, "netapi32.lib")

static wxString getFullUserName()
{
    WKSTA_USER_INFO_1* wksta_user_info;
    USER_INFO_2* ui;
    LPWSTR pdc_computer;
    DWORD cnt = 254;

    // -- get login domain and user id --
    if (NERR_Success!= NetWkstaUserGetInfo(NULL, 1, (LPBYTE*)&wksta_user_info))
    {
        return ::wxGetUserName();
    }

    // -- get computer name of the domain controller --
    if (NERR_Success != NetGetDCName(NULL,
                                     wksta_user_info->wkui1_logon_domain,
                                     (LPBYTE*)&pdc_computer))
    {
        pdc_computer = NULL;
    }


    if (NERR_Success != NetUserGetInfo(pdc_computer,
                                       wksta_user_info->wkui1_username,
                                       2,
                                       (LPBYTE*)&ui))
    {
        if (pdc_computer)
        {
            NetApiBufferFree(pdc_computer);
        }

        NetApiBufferFree(wksta_user_info);
        return ::wxGetUserName();
    }

    wxString ret = ui->usri2_full_name;

    if (pdc_computer)
    {
        NetApiBufferFree(pdc_computer);
    }

    NetApiBufferFree(wksta_user_info);
    NetApiBufferFree(ui);

    return ret;
}

#else

static wxString getFullUserName()
{
    return ::wxGetUserName();
}

#endif

int InetAuth::authorize(paladin::Authentication* auth,
                        const wxString& product_version,
                        const wxString& license_login,
                        const wxString& license_password)
{
    // get product_tag and site_code from paladin
    wxString product_tag = towx(auth->getAppTag());
    paladin::sitecode_t code = auth->getSiteCode();
    char szcode[255];
    paladin::getStringFromCode(code, szcode, false);
    wxString site_code = wxString::From8BitData(szcode);

    // create a check variable.  A check variable isa CRC32 of the site code.
    // This will make undesired manual access to this url more difficult

    unsigned int crc = paladin::crc32((unsigned char*)szcode, strlen(szcode));
    wxString check;
    check.Printf(wxT("%08x"), crc);

/*
#ifdef _DEBUG
    wxTextEntryDialog dlg(NULL, _("Please enter computer name:"));
    if (dlg.ShowModal() != wxID_OK)
        return errorNetworkError;
    if (dlg.GetValue().Length() > 0)
        host_name = dlg.GetValue();
#endif
*/

    wxString host_name = ::wxGetHostName();
    wxString auth_server = getAuthServer();
    
    wxString urlstring;
    urlstring += auth_server;
    urlstring += wxT("?");

    addGetParam(urlstring, wxT("inetact_version"), wxT("2"));
    addGetParam(urlstring, wxT("command"), wxT("activate"));
    addGetParam(urlstring, wxT("product_tag"), product_tag);
    addGetParam(urlstring, wxT("product_version"), product_version);
    addGetParam(urlstring, wxT("serial"), license_login);
    addGetParam(urlstring, wxT("password"), license_password);
    addGetParam(urlstring, wxT("site_code"), site_code);
    addGetParam(urlstring, wxT("check"), check);

#ifdef WIN32
    // -- for Terminal Server and Citrix environments,
    //    we will enforce per-user licensing --

    // 0x1000 = SM_REMOTESESSION
    if (GetSystemMetrics(0x1000))
    {
        addGetParam(urlstring, wxT("auth_mode"), wxT("U"));
    }
#endif



/*
#ifdef WIN32
    // for Terminal Server and Citrix environments,
    // we will enforce per-user licensing

    // 0x1000 = SM_REMOTESESSION
    if (GetSystemMetrics(0x1000 ))
    {
        addGetParam(urlstring, wxT("auth_mode"), wxT("U"));
    }
#endif
*/
    
    
    wxString result;
    result = getWebFile(urlstring);

    if (result.IsEmpty())
    {
        return errorNetworkError;
    }

    return installLicense(auth, result);
}


int InetAuth::deauthorize(paladin::Authentication* auth,
                          const wxString& license_login,
                          const wxString& license_password)
{
    // -- get product_tag and site_code from paladin --
    wxString product_tag = towx(auth->getAppTag());
    paladin::sitecode_t code = auth->getSiteCode();
    char szcode[255];
    paladin::getStringFromCode(code, szcode, false);
    wxString site_code = wxString::From8BitData(szcode);

    // -- create a check variable.  A check variable is
    //    a CRC32 of the site code.  This will prevent
    //    undesired manual access to this url --

    unsigned int crc = paladin::crc32((unsigned char*)szcode, strlen(szcode));
    wxString check;
    check.Printf(wxT("%08X"), crc);



    wxString host_name = ::wxGetHostName();
    wxString auth_server = getAuthServer();
    
    wxString urlstring;
    urlstring += auth_server;
    urlstring += wxT("?");

    addGetParam(urlstring, wxT("inetauth_version"), wxT("1"));
    addGetParam(urlstring, wxT("command"), wxT("deauthorize"));
    addGetParam(urlstring, wxT("product_tag"), product_tag);
    addGetParam(urlstring, wxT("site_code"), site_code);
    addGetParam(urlstring, wxT("computer_name"), host_name);
    addGetParam(urlstring, wxT("user_id"), ::wxGetUserId());
    addGetParam(urlstring, wxT("license_login"), license_login);
    addGetParam(urlstring, wxT("license_password"), license_password);
    addGetParam(urlstring, wxT("check"), check);

#ifdef WIN32
    // -- for Terminal Server and Citrix environments,
    //    we will enforce per-user licensing --

    if (GetSystemMetrics(0x1000 /*SM_REMOTESESSION*/))
    {
        addGetParam(urlstring, wxT("auth_mode"), wxT("U"));
    }
#endif
    
 


    
    wxString result;
    result = getWebFile(urlstring);
    
    //cfw::appMessageBox(result, urlstring);

    if (result.IsEmpty())
    {
        return errorNetworkError;
    }


    // -- determine whether there was an error or not --

    kl::xmlnode root;
    if (!root.parse(towstr(result)))
        return errorFailure;

    kl::xmlnode& error_code = root.getChild(L"error_code");


    if (error_code.getNodeValue() != L"success")
        return errorFailure;


    // -- actually deauthorize this computer --

    auth->deactivate();

    return errorSuccess;
}




