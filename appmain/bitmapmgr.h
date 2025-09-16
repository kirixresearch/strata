/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2002-01-22
 *
 */


#ifndef H_APP_BITMAPMGR_H
#define H_APP_BITMAPMGR_H


#include <map>


#define GETBMP(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal)
#define GETDISBMP(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled)

#define GETBMPSMALL(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal, BitmapMgr::sizeSmallIcon)
#define GETDISBMPSMALL(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled, BitmapMgr::sizeSmallIcon)

#define GETBMPMEDIUM(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal, BitmapMgr::sizeMediumIcon)
#define GETDISBMPMEDIUM(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled, BitmapMgr::sizeMediumIcon)

#define GETBMPLARGE(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeNormal, BitmapMgr::sizeLargeIcon)
#define GETDISBMPLARGE(bitmap_name) BitmapMgr::getBitmap(wxT(#bitmap_name), BitmapMgr::typeDisabled, BitmapMgr::sizeLargeIcon)


#define ID2BMP(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal)
#define ID2BMPSMALL(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, BitmapMgr::sizeSmallIcon)
#define ID2BMPMEDIUM(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, BitmapMgr::sizeMediumIcon)
#define ID2BMPLARGE(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, BitmapMgr::sizeLargeIcon)

#define ID2DISBMP(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeDisabled)
#define ID2DISBMPSMALL(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeDisabled, BitmapMgr::sizeSmallIcon)
#define ID2DISBMPMEDIUM(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, BitmapMgr::sizeMediumIcon)
#define ID2DISBMPLARGE(command_id) BitmapMgr::getBitmap(command_id, BitmapMgr::typeNormal, BitmapMgr::sizeLargeIcon)

class BitmapEntry
{
public:
    wxString name;
    char** data;
    wxBitmap bitmap;
    wxBitmap bitmap16;
    wxBitmapBundle bitmapBundle;
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

    enum
    {
        sizeSmallIcon = -100,   // 16px, or 24/32px on higher dpi/resolution displays
        sizeMediumIcon = -200,  // 24px, or 32/48px on higher dpi/resolution displays
        sizeLargeIcon = -300    // 32px, or 48/64px on higher dpi/resolution displays
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
    
    wxBitmapBundle lookupBitmapBundle(const wxString& bitmap_name,
        int type = typeNormal,
        int size = -1);

    wxBitmapBundle lookupBitmapBundle(int command_id,
        int type = typeNormal,
        int size = -1);

    void addIdBitmapMap(int command_id, const wxString& bitmap_name);
    
public:

    static void initBitmapMgr();
    static BitmapMgr* getBitmapMgr();
    static void uninitBitmapMgr();
    static int getBitmapIconSize(int size_id);

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
