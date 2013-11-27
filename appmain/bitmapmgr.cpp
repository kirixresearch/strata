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
    wxString name = bitmap_name;

    BitmapEntry& entry = m_entries[name];
    if (!entry.bitmap.Ok())
    {
        wxString entry_base = name;
        wxString entry_name = entry_base;
        wxBitmapType entry_type = wxBITMAP_TYPE_PNG;
        
        if (name.Left(3) == wxT("xpm"))
        {
            entry_name += wxT(".xpm");
            entry_type = wxBITMAP_TYPE_XPM;
        }
         else
        {
            entry_name += wxT(".png");
        }
    
        // load the bitmap from the catalog
        wxMemoryBuffer buf;
        if (!m_bundle->getEntry(entry_name, buf))
        {
            // if png and xpm fail, try gif
            entry_name = entry_base;
            entry_name += wxT(".gif");
            entry_type = wxBITMAP_TYPE_GIF;
            
            if (!m_bundle->getEntry(entry_name, buf))
            {
                wxFAIL_MSG(wxT("Image not found in imgres.zip"));
                return wxBitmap();
            }
        }
    
        wxMemoryInputStream stream(buf.GetData(), buf.GetDataLen());
        
        wxImage img;
        if (!img.LoadFile(stream, entry_type))
        {
            wxFAIL_MSG(wxT("Image failed to load"));
            return wxBitmap();
        }
        
        entry.bitmap = wxBitmap(img);
    }
    
    
    if (size == -1 || (size == entry.bitmap.GetWidth() &&
                       size == entry.bitmap.GetHeight()))
    {
        if (status == typeDisabled)
            return MakeDisabledBitmap(entry.bitmap);
            
        return entry.bitmap;
    }
        
    if (size == 16)
    {
        // if the 16x16 bitmap exists, return it
        if (entry.bitmap16.IsOk())
        {
            if (status == typeDisabled)
                return MakeDisabledBitmap(entry.bitmap16);

            return entry.bitmap16;
        }
        
        // try to look up the 16x16 bitmap
        wxString bmp16_name = bitmap_name.BeforeLast(wxT('_'));
        bmp16_name += wxT("_16");
        entry.bitmap16 = lookupBitmap(bmp16_name, status, size);
        if (entry.bitmap16.IsOk())
        {
            if (status == typeDisabled)
                return MakeDisabledBitmap(entry.bitmap16);

            return entry.bitmap16;
        }
        
        // no 16x16 bitmap exists, so resize the larger-sized bitmap
        wxImage image = entry.bitmap.ConvertToImage();
        image.Rescale(16,16);
        entry.bitmap16 = wxBitmap(image);

        if (status == typeDisabled)
            return MakeDisabledBitmap(entry.bitmap);

        return entry.bitmap16;
    }

    if (status == typeDisabled)
        return MakeDisabledBitmap(entry.bitmap);

    return entry.bitmap;
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
