/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-22
 *
 */


#ifndef __APP_BITMAPMGR_H
#define __APP_BITMAPMGR_H


#include <map>


#define GETBMP(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal)
#define GETDISBMP(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled)

#define GETBMPSMALL(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal | BitmapMgr::typeAppendSize, BitmapMgr::getSmallIconSize())
#define GETDISBMPSMALL(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled | BitmapMgr::typeAppendSize, BitmapMgr::getSmallIconSize())

#define GETBMPMEDIUM(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal | BitmapMgr::typeAppendSize, BitmapMgr::getMediumIconSize())
#define GETDISBMPMEDIUM(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled | BitmapMgr::typeAppendSize, BitmapMgr::getMediumIconSize())


#define ID2BMP(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal)
#define ID2BMP16(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, 16)

#define ID2DISBMP(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeDisabled)
#define ID2DISBMP16(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeDisabled, 16)


class BitmapEntry
{
public:
    wxString name;
    char** data;
    wxBitmap bitmap;
    wxBitmap bitmap16;
};

class ZipBundle;
class BitmapMgr
{
public:

    enum
    {
        typeNormal = 0,
        typeDisabled = 1,
        typeHot = 2,
        typeAppendSize = 4
    };
    
public:

    BitmapMgr();
    ~BitmapMgr();
    
    bool init(const wxString& filename);
    void clear();
    
    wxBitmap lookupBitmap(const wxString& bitmap_name,
                          int type = typeNormal,
                          int size = -1);
                          
    wxBitmap lookupBitmap(int command_id,
                          int type = typeNormal,
                          int size = -1);
    
    void addIdBitmapMap(int command_id, const wxString& bitmap_name);
    
public:

    static void initBitmapMgr();
    static BitmapMgr* getBitmapMgr();
    static void uninitBitmapMgr();
    static int getSmallIconSize();
    static int getMediumIconSize();

    // calls to this function will not
    // add the bitmap to the bitmap manager
    static wxBitmap makeDisabledBitmap(const wxBitmap& bitmap);
    
    static wxBitmap getBitmap(const wxString& bitmap_name,
                              int type = typeNormal,
                              int size = -1);
                              
    static wxBitmap getBitmap(int command_id,
                              int type,
                              int size = -1);
    
private:
    std::map<wxString, BitmapEntry> m_entries;
    std::map<int, wxString> m_idmap;
    ZipBundle* m_bundle;
};




#endif
