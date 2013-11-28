
#include <wx/wx.h>
#include <wx/mstream.h>
#include <map>




#define REGISTER(img) g_bitmap_mgr.registerImage(#img, img ## _png, img ## _png_size);
void registerImages();
class BitmapMgr
{
public:

    BitmapMgr()
    {
        
        ::registerImages();
    }

    void registerImage(const char* img, const unsigned char* data, size_t size)
    {
        m_images[img] = std::pair<const unsigned char*, size_t>(data, size);
    }

    wxBitmap lookupBitmap(const wxString& name)
    {
        std::map<wxString, wxBitmap>::iterator it;
    
        it = m_bitmaps.find(name);
        if (it != m_bitmaps.end())
            return it->second;

        std::map<wxString, std::pair<const unsigned char*, size_t> >::iterator iit;
        iit = m_images.find(name);
        if (iit == m_images.end())
            return wxNullBitmap;

        wxMemoryInputStream stream(iit->second.first, iit->second.second);
        wxImage img(stream, wxBITMAP_TYPE_PNG);
        if (!img.IsOk())
            return wxNullBitmap;

        wxBitmap bmp(img);
        m_bitmaps[name] = bmp;

        return bmp;
    }

    std::map<wxString, std::pair<const unsigned char*, size_t> > m_images;
    std::map<wxString, wxBitmap> m_bitmaps;
};
BitmapMgr g_bitmap_mgr;




#include "../img/c/gf_checkmark_32.png.c"
#include "../img/c/gf_db_conn_blue_24.png.c"
#include "../img/c/gf_gear_24.png.c"

void registerImages()
{
    REGISTER(gf_checkmark_32)
    REGISTER(gf_db_conn_blue_24)
    REGISTER(gf_gear_24)
}

wxBitmap lookupBitmap(const wxString& name)
{
    return g_bitmap_mgr.lookupBitmap(name);
}



