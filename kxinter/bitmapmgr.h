/*!
 *
 * Copyright (c) 2002-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Data Interchange Application
 * Author:   Benjamin I. Williams
 * Created:  2002-01-22
 *
 */


#ifndef __KXINTER_BITMAPMGR_H
#define __KXINTER_BITMAPMGR_H


#define GET_XPM(x) BitmapMgr::getBitmap(wxT(#x))
#define REGISTER_XPM(x) BitmapMgr::getBitmapMgr()->registerXpmBitmap(wxT(#x), x)


class BitmapEntry
{
public:
    wxString name;
    char** data;
    wxBitmap bitmap;
};

class BitmapMgr
{
private:
    std::vector<BitmapEntry> m_entries;

public:

    static BitmapMgr* getBitmapMgr(bool destroy = false)
    {
        static BitmapMgr* b = NULL;

        if (destroy)
        {
            if (b)
            {
                delete b;
                b = NULL;
                return NULL;
            }
        }

        if (!b)
        {
            b = new BitmapMgr;
        }

        return b;
    }

    static wxBitmap getBitmap(const wxString& bitmap_name)
    {
        return getBitmapMgr()->lookupBitmap(bitmap_name);
    }

public:

    void clear()
    {
        m_entries.clear();
    }

    void registerXpmBitmap(const wxString& bitmap_name, char** data)
    {
        BitmapEntry entry;
        entry.name = bitmap_name;
        entry.data = data;
        m_entries.push_back(entry);
    }

    wxBitmap lookupBitmap(const wxString& bitmap_name)
    {
        for (std::vector<BitmapEntry>::iterator it = m_entries.begin();
                it != m_entries.end(); ++it)
        {
            if (!it->name.CmpNoCase(bitmap_name))
            {
                if (it->bitmap.Ok())
                {
                    return it->bitmap;
                }

                it->bitmap = wxBitmap(it->data);
                return it->bitmap;
            }
        }

        wxBitmap empty;
        return empty;
    }

};




#endif
