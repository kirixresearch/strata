/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-07
 *
 */


#ifndef __APP_BOOKMARK_H
#define __APP_BOOKMARK_H


class Bookmark
{
public:
    
    Bookmark();
    ~Bookmark();
    
    // this function allows the creation of both mount and link bookmark
    // items and replaces the need for AppController::createBookmark()
    static void create(const wxString& path,
                       const wxString& loc,
                       const wxString& tags = wxEmptyString,
                       const wxString& desc = wxEmptyString,
                       const wxImage& favicon = wxImage());
    
    // helper functions to encode and decode bookmark images
    static wxImage textToImage(const wxString& str);
    static wxString imageToText(wxImage& image);
   
    bool load(const wxString& path);
    bool save(const wxString& path);

    void setLocation(const wxString& loc);
    void setTags(const wxString& tags);
    void setDescription(const wxString& desc);
    void setFavIcon(const wxImage& favicon);
    void setRunTarget(bool new_value);
    
    wxString getLocation()          { return m_loc;  }
    wxString getTags()              { return m_tags; }
    wxString getDescription()       { return m_desc; }
    wxImage getFavIcon()            { return m_favicon; }
    bool getRunTarget()             { return m_run_target; }
    
private:

    wxString m_path;
    wxString m_loc;
    wxString m_tags;
    wxString m_desc;
    bool m_run_target;
    wxImage m_favicon;
};


#endif


