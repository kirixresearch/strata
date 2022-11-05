/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2005-07-20
 *
 */


#include "appmain.h"
#include "bitmapmgr.h"
#include <wx/zipstrm.h>
#include <wx/ffile.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>


BitmapMgr* g_bitmapmgr = NULL;


class ZipBundle
{
public:


    ZipBundle()
    {
    }
    
    ~ZipBundle()
    {
        // free up all catalog entries
        std::map<wxString, wxZipEntry*>::iterator it, end;
        end = m_catalog.end();
        for (it = m_catalog.begin(); it != end; ++it)
            delete it->second;
        
        if (m_f.IsOpened())
            m_f.Close();
    }

    bool init(const wxString& filename)
    {
        if (!wxFile::Exists(filename))
            return false;
            
        if (!m_f.Open(filename))
            return false;
            
        wxFileInputStream in(m_f);
        wxZipInputStream zip(in);
        
        std::map<wxString, wxZipEntry*>::iterator it;
        
        wxZipEntry* entry;
        
        // load the zip catalog
        while ((entry = zip.GetNextEntry()) != NULL)
        {
            wxString name = entry->GetInternalName();
            
            it = m_catalog.find(name);
            if (it != m_catalog.end())
            {
                // name already exists in catalog;
                // override it with the new name
                delete it->second;
            }
            
            m_catalog[name] = entry;
        }

        return true;
    }
    
    bool getEntry(const wxString& path, wxMemoryBuffer& buf)
    {
        // initialize output buffer size
        buf.SetDataLen(0);
        
        // find entry
        std::map<wxString, wxZipEntry*>::iterator it;
        wxString name = wxZipEntry::GetInternalName(path);
        
        size_t s = m_catalog.size();
        
        it = m_catalog.find(name);
        if (it == m_catalog.end())
            return false;
        
        if (!m_f.IsOpened())
            return false;
            
        wxFileInputStream in(m_f);
        wxZipInputStream zip(in);

        if (!zip.OpenEntry(*(it->second)))
            return false;
        
        while (!zip.Eof())
        {
            void* ptr = buf.GetAppendBuf(10000);
            zip.Read(ptr, 10000);
            buf.UngetAppendBuf(zip.LastRead());
        }
        
        zip.CloseEntry();
        
        return true;
    }

private:

    wxFile m_f;
    std::map<wxString, wxZipEntry*> m_catalog;
};



static double BlendColour(double fg, double bg, double alpha)
{
    double result = bg + (alpha * (fg - bg));
    if (result < 0.0)
        result = 0.0;
    if (result > 255)
        result = 255;
    return result;
}

static wxBitmap MakeDisabledBitmap(const wxBitmap& bmp)
{
    wxImage image = bmp.ConvertToImage();
    
    int mr, mg, mb;
    mr = image.GetMaskRed();
    mg = image.GetMaskGreen();
    mb = image.GetMaskBlue();

    unsigned char* data = image.GetData();
    int width = image.GetWidth();
    int height = image.GetHeight();
    bool has_mask = image.HasMask();

    for (int y = height-1; y >= 0; --y)
    {
        for (int x = width-1; x >= 0; --x)
        {
            data = image.GetData() + (y*(width*3))+(x*3);
            unsigned char* r = data;
            unsigned char* g = data+1;
            unsigned char* b = data+2;

            if (has_mask && *r == mr && *g == mg && *b == mb)
                continue;

            *r = (unsigned char)BlendColour((double)*r, 255.0, 0.4);
            *g = (unsigned char)BlendColour((double)*g, 255.0, 0.4);
            *b = (unsigned char)BlendColour((double)*b, 255.0, 0.4);
        }
    }

    return wxBitmap(image);
}



BitmapMgr::BitmapMgr()
{
    m_bundle = new ZipBundle;
}

BitmapMgr::~BitmapMgr()
{
    delete m_bundle;
}


bool BitmapMgr::init(const wxString& filename)
{
    return m_bundle->init(filename);
}


void BitmapMgr::clear()
{
    m_entries.clear();
}

void BitmapMgr::initBitmapMgr()
{
    if (g_bitmapmgr)
        return;
        
    g_bitmapmgr = new BitmapMgr;
}

void BitmapMgr::uninitBitmapMgr()
{
    delete g_bitmapmgr;
}

int BitmapMgr::getBitmapIconSize(int size_id)
{
    static int small_size = -1, medium_size = -1, large_size = -1;
    
    switch (size_id)
    {
        default:
        case BitmapMgr::sizeSmallIcon:
            if (small_size == -1)
            {
                small_size = wxTheApp->GetMainTopWindow()->FromDIP(100) <= 100 ? 16 : 24;
            }
            return small_size;
        case BitmapMgr::sizeMediumIcon:
            if (medium_size == -1)
            {
                medium_size = wxTheApp->GetMainTopWindow()->FromDIP(100) <= 100 ? 24 : 32;
            }
            return medium_size;
        case BitmapMgr::sizeLargeIcon:
            if (large_size == -1)
            {
                large_size = wxTheApp->GetMainTopWindow()->FromDIP(100) <= 100 ? 32 : 48;
            }
            return large_size;
    }
}


BitmapMgr* BitmapMgr::getBitmapMgr()
{
    if (!g_bitmapmgr)
        initBitmapMgr();
    return g_bitmapmgr;
}

wxBitmap BitmapMgr::makeDisabledBitmap(const wxBitmap& bitmap)
{
    return MakeDisabledBitmap(bitmap);
}

wxBitmap BitmapMgr::getBitmap(const wxString& bitmap_name, int status, int size)
{
    return getBitmapMgr()->lookupBitmap(bitmap_name, status, size);
}

wxBitmap BitmapMgr::getBitmap(int id, int status, int size)
{
    return getBitmapMgr()->lookupBitmap(id, status, size);
}

wxBitmap BitmapMgr::lookupBitmap(const wxString& bitmap_name, int status, int size)
{
    const int sizes_count = 5;
    static int sizes[sizes_count] = { 64, 48, 32, 24, 15 };

    wxString name = bitmap_name;

    int desired_size = -1;
    if (name.EndsWith("_16"))
    {
        desired_size = 16;
        name = name.Left(bitmap_name.Length() - 3);
    }
    else if (name.EndsWith("_24"))
    {
        desired_size = 24;
        name = name.Left(bitmap_name.Length() - 3);
    }
    else if (name.EndsWith("_32"))
    {
        desired_size = 32;
        name = name.Left(bitmap_name.Length() - 3);
    }
    else if (name.EndsWith("_48"))
    {
        desired_size = 48;
        name = name.Left(bitmap_name.Length() - 3);
    }

    if (size == BitmapMgr::sizeSmallIcon || size == BitmapMgr::sizeMediumIcon || size == BitmapMgr::sizeLargeIcon)
    {
        size = getBitmapIconSize(size);
    }

    if (size == -1)
    {
        size = (desired_size == -1 ? getBitmapIconSize(BitmapMgr::sizeSmallIcon) : desired_size);
    }


    wxString key = name + ";" + wxString::Format("%d", size) + ";" + (status == typeDisabled ? "d" : "n");

    BitmapEntry& entry = m_entries[key];
    if (entry.bitmap.Ok())
    {
        return entry.bitmap;
    }
    else
    {
        wxMemoryBuffer buf;
        bool res;
        wxBitmapType entry_type = wxBITMAP_TYPE_PNG;

        // load the bitmap from the catalog -- first try the exact size wanted;
        // if the precise size is not available, load the largest available and
        // scale it up or down
        res = m_bundle->getEntry(name + wxString::Format("_%d.png", size), buf);
        if (!res)
        {
            // try different sizes and scale
            for (int i = 0; i < sizes_count; ++i)
            {
                res = m_bundle->getEntry(name + wxString::Format("_%d.png", sizes[i]), buf);
                if (res)
                {
                    break;
                }
            }
        }


        if (!res)
        {
            wxFAIL_MSG("Image not found in imgres.zip");
            return wxBitmap();
        }

        wxMemoryInputStream stream(buf.GetData(), buf.GetDataLen());
        
        wxImage img;
        if (!img.LoadFile(stream, entry_type))
        {
            wxFAIL_MSG("Image failed to load");
            return wxBitmap();
        }
        
        if (img.GetHeight() != size)
        {
            img.Rescale(size, size);
        }

        entry.bitmap = wxBitmap(img);

        if (status == typeDisabled)
        {
            entry.bitmap = MakeDisabledBitmap(entry.bitmap);
        }

        return entry.bitmap;
    }
}

wxBitmap BitmapMgr::lookupBitmap(int id,
                                 int status,
                                 int size)
{
    return lookupBitmap(m_idmap[id], status, size);
}

void BitmapMgr::addIdBitmapMap(int command_id,
                               const wxString& bitmap_name)
{
    m_idmap[command_id] = bitmap_name;
}
