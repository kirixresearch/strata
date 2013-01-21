/*!
 *
 * Copyright (c) 2004-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Paladin Code Generator Client
 * Author:   Benjamin I. Williams
 * Created:  2004-05-08
 *
 */


#include <wx/wx.h>
#include <wx/datetime.h>
#include "app.h"
#include "../paladin/paladin.h"
#include "../paladin/crypt.h"
#include "../paladin/crc.h"
#include "../paladin/util.h"


// -- our "generic" key (don't change) --
static unsigned char generic_key[] = { 0x42, 0x49, 0x98, 0xba, 0x22, 0x34, 0x32, 0x8e };


static int dateToJulian(int year, int month, int day)
{
    int a, y, m;

    a = (14-month)/12;
    y = year + 4800 - a;
    m = month + (12 * a) - 3;
    return (day + (((153*m)+2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045);
}

static paladin::sitecode_t getCodeFromString(const wxString& input_site_code)
{
    paladin::sitecode_t site_code = 0;
    paladin::sitecode_t multiplier = 1;

    static const wxChar* hexchars = wxT("0123456789ABCDEF");

    int counter = 0;

    wxString s = input_site_code;
    s.MakeUpper();
    s.Trim();

    if (s.Length() == 0)
    {
        return 0;
    }

    int site_code_len = 0;

    while (!s.IsEmpty())
    {
        wxChar c = s.Last();
        s.RemoveLast();

        if (wxIsspace(c) || c == wxT('-'))
            continue;

        const wxChar* pos = wxStrchr(hexchars, c);
        if (!pos)
        {
            site_code = 0;
            break;
        }

        site_code_len++;

        paladin::sitecode_t a = pos-hexchars;
        site_code += (a*multiplier);
        multiplier *= 16;

        if (++counter > 16)
        {
            site_code = 0;
            break;
        }
    }
    
    if (site_code_len != 16)
        return 0;

    return site_code;
}


bool validateSiteCode(const wxString& val_site_code)
{
    paladin::sitecode_t site_code = getCodeFromString(val_site_code);

    // decrypt the side code
    paladin::int64crypt(site_code, generic_key, false);

    unsigned char check_byte = (unsigned char)(site_code >> 56);
    
#ifdef _MSC_VER
    site_code &= 0x00ffffffffffffff;
#else
    site_code &= 0x00ffffffffffffffLL;
#endif

    if ((site_code % 0xff) == check_byte)
        return true;
        
    return false;
}


wxString calcActivationCode(const wxString& input_app_tag,
                            const wxString& input_site_code,
                            unsigned int input_feature_id,
                            int exp_year,
                            int exp_month,
                            int exp_day)
{
    unsigned char actcode[8];

    paladin::sitecode_t site_code = 0;

    memset(actcode, 0, 8);


    // -- get site code that user entered --

    site_code = getCodeFromString(input_site_code);
    
    if (site_code == 0)
    {
        return wxT("");
    }
    
    // -- prepare site_code and host_id code byte arrays --

    unsigned char site_code_bytes[8];
    unsigned char host_id_bytes[8];

    paladin::int64unpack(site_code_bytes, site_code);


    // -- get the site's host id from the site's site code --

    paladin::paladin_int64_t unenc_site_code = site_code;
    paladin::int64crypt(unenc_site_code, generic_key, false);


#ifdef _MSC_VER
    unenc_site_code &= 0xffffffffffff;
#else
    unenc_site_code &= 0xffffffffffffLL;
#endif


    paladin::sitecode_t host_id = unenc_site_code;
    paladin::int64unpack(host_id_bytes, host_id);


    // -- calculate activation code --

    //wxDateTime dt(exp_day, (wxDateTime::Month)(exp_month-1), exp_year);
    //unsigned int expire_julian = dateToJulian(dt.GetYear(), dt.GetMonth()+1, dt.GetDay()) - dateToJulian(2000, 1, 1);

    wxDateTime today = wxDateTime::Today();
    unsigned int crc = paladin::crc16(host_id_bytes, 8);
    unsigned int createdt_julian = dateToJulian(today.GetYear(), today.GetMonth()+1, today.GetDay()) - dateToJulian(2000, 1, 1);
    unsigned int feature_id = input_feature_id;

    unsigned int expire_julian = 0;

    if (exp_year == 0 && exp_month == 0 && exp_day == 0)
    {
        // -- user requests a perpetual (non-expiring) license,
        //    which is denoted by 0xffaa as the expire julian --

        expire_julian = 0xffaa;
    }
     else
    {
        expire_julian = dateToJulian(exp_year, exp_month, exp_day) - dateToJulian(2000, 1, 1);
    }

    actcode[0] = (crc) & 0xff;
    actcode[1] = (crc >> 8) & 0xff;
    actcode[2] = (expire_julian) & 0xff;
    actcode[3] = (expire_julian >> 8) & 0xff;
    actcode[4] = (createdt_julian) & 0xff;
    actcode[5] = (createdt_julian >> 8) & 0xff;
    actcode[6] = (feature_id) & 0xff;
    actcode[7] = (feature_id >> 8) & 0xff;

    // -- encrypt site code with the master encryption key --

    Des d;

    unsigned char master_key[8];

    char buf[255];
    strcpy(buf, (const char*)input_app_tag.mbc_str());
    if (strlen(buf) == 0)
    {
        return wxT("");
    }

    unsigned long app_tag_crc = paladin::crc32((unsigned char*)buf, strlen(buf));

    master_key[0] = (app_tag_crc) & 0xff;
    master_key[1] = (app_tag_crc >> 8) & 0xff;
    master_key[2] = (app_tag_crc >> 16) & 0xff;
    master_key[3] = (app_tag_crc >> 24) & 0xff;
    master_key[4] = 0x8a;
    master_key[5] = 0x4b;
    master_key[6] = 0x22;
    master_key[7] = 0x1f;


    unsigned char enc_key[8];
    memcpy(enc_key, host_id_bytes, 8);
    d.setKey(master_key);
    d.crypt(enc_key, 8, true);

    // -- encrypt activation code --
    d.setKey(enc_key);
    d.crypt(actcode, 8, true);

    // -- encrypt activation with site key --
    d.setKey(site_code_bytes);
    d.crypt(actcode, 8, true);


    // -- generate final activation code from buffer --

    paladin::actcode_t final_actcode = 0;
    paladin::int64pack(final_actcode, actcode);


    // -- put activation code into output textctrl --

    wxString actcode_str;

    unsigned long hi, lo;
    hi = (final_actcode >> 32) & 0xffffffff;
    lo = (final_actcode & 0xffffffff);

    unsigned long hiw, low;

    hiw = (hi >> 16) & 0xffff;
    low = (hi & 0xffff);
    actcode_str += wxString::Format(wxT("%04X %04X "), hiw, low);
    hiw = (lo >> 16) & 0xffff;
    low = (lo & 0xffff);
    actcode_str += wxString::Format(wxT("%04X %04X"), hiw, low);

    return actcode_str;
}


wxString calcComputerId(const wxString& input_site_code)
{
    unsigned char actcode[8];
    const wxChar* hexchars = wxT("0123456789ABCDEF");

    paladin::sitecode_t site_code = 0;
    paladin::sitecode_t multiplier = 1;

    memset(actcode, 0, 8);


    // -- get site code that user entered --

    int counter = 0;

    wxString s = input_site_code;
    s.MakeUpper();
    s.Trim();

    if (s.Length() == 0)
    {
        return wxT("");
    }

    while (!s.IsEmpty())
    {
        wxChar c = s.Last();
        s.RemoveLast();

        if (wxIsspace(c) || c == wxT('-'))
            continue;

        const wxChar* pos = wxStrchr(hexchars, c);
        if (!pos)
        {
            site_code = 0;
            break;
        }

        paladin::sitecode_t a = pos-hexchars;
        site_code += (a*multiplier);
        multiplier *= 16;

        if (++counter > 16)
        {
            site_code = 0;
            break;
        }
    }
    
    if (site_code == 0)
    {
        return wxT("");
    }


    // -- prepare site_code and host_id code byte arrays --

    unsigned char site_code_bytes[8];

    paladin::int64unpack(site_code_bytes, site_code);


    // -- get the site's host id from the site's site code --

    paladin::paladin_int64_t unenc_site_code = site_code;
    paladin::int64crypt(unenc_site_code, generic_key, false);


#ifdef _MSC_VER
    unenc_site_code &= 0xffffffffffff;
#else
    unenc_site_code &= 0xffffffffffffLL;
#endif


    // -- put activation code into output textctrl --

    wxString ret_str;

    unsigned long hi, lo;
    hi = (unenc_site_code >> 32);
    lo = (unenc_site_code & 0xffffffff);

    unsigned long hiw, low;

    hiw = (hi >> 16);
    low = (hi & 0xffff);
    ret_str += wxString::Format(wxT("%04X %04X "), hiw, low);
    hiw = (lo >> 16);
    low = (lo & 0xffff);
    ret_str += wxString::Format(wxT("%04X %04X"), hiw, low);

    return ret_str;
}
