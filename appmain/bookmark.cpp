/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2007-11-07
 *
 */


#include "appmain.h"
#include "bookmark.h"
#include "jsonconfig.h"
#include <wx/sstream.h>
#include <wx/buffer.h>
#include <wx/mstream.h>


static void bin2hex(const unsigned char* data, size_t len, wxString& output)
{
    output.reserve((len*2) + 1);
    output = wxT("X"); // version
    
    wxString s;
    for (size_t i = 0; i < len; ++i)
    {
        s.Printf(wxT("%02X"), data[i]);
        output += s;
    }
}


inline unsigned char hex2byte(wxChar b1, wxChar b2)
{
    if (b1 >= '0' && b1 <= '9')
        b1 -= '0';
    else if (b1 >= 'A' && b1 <= 'F')
        b1 = b1 - 55;
    else
        return 0;
    
    if (b2 >= '0' && b2 <= '9')
        b2 -= '0';
    else if (b2 >= 'A' && b2 <= 'F')
        b2 = b2 - 55;
    else
        return 0;
    
    return (b1*16)+b2;
}

void hex2bin(const wxString& _input, wxMemoryBuffer& output)
{
    wxString input = _input;
    input.Trim(false);
    input.Trim(true);
    
    output.SetBufSize(0);
    
    if (input.Length() <= 1)
        return;
    
    wxChar ch1, ch2;
    const wxChar* buf = input.c_str();
    
    if (*buf != 'X') // check version
        return;
    buf++;
    
    output.SetBufSize((input.Length()-1)/2);
    
    while (*buf)
    {
        ch1 = towupper(*buf);
        ++buf;
        
        if (!*buf)
            break;
        ch2 = towupper(*buf);
    
        output.AppendByte((char)hex2byte(ch1, ch2));
    
        ++buf;
    }
}

// static
wxImage Bookmark::textToImage(const wxString& str)
{
    wxMemoryBuffer buf;
    hex2bin(str, buf);
    
    wxMemoryInputStream input_stream(buf.GetData(), buf.GetDataLen());
    
    wxImage img;
    if (buf.GetDataLen() == 0)
        return img;
        
    img.LoadFile(input_stream, wxBITMAP_TYPE_PNG);
    return img;
}

// static
wxString Bookmark::imageToText(wxImage& image)
{
    wxMemoryOutputStream output_stream;
    image.SaveFile(output_stream, wxBITMAP_TYPE_PNG);
    
    size_t len = output_stream.GetLength();
    wxMemoryBuffer buf;
    output_stream.CopyTo(buf.GetAppendBuf(len), len);
    buf.UngetAppendBuf(len);
    
    wxString ret;
    bin2hex((const unsigned char*)buf.GetData(), buf.GetDataLen(), ret);
    return ret;
}



Bookmark::Bookmark()
{
    m_path = wxEmptyString;
    m_loc  = wxEmptyString;
    m_tags = wxEmptyString;
    m_desc = wxEmptyString;
    m_run_target = false;
}

Bookmark::~Bookmark()
{

}

void Bookmark::setLocation(const wxString& loc)
{
    m_loc = loc;
    m_loc.Trim(false).Trim(true);
}

void Bookmark::setTags(const wxString& tags)
{
    m_tags = tags;
    m_tags.Trim(false).Trim(true);
}

void Bookmark::setDescription(const wxString& desc)
{
    m_desc = desc;
}

void Bookmark::setFavIcon(const wxImage& favicon)
{
    m_favicon = favicon;
}

void Bookmark::setRunTarget(bool new_value)
{
    m_run_target = new_value;
}

void Bookmark::create(const wxString& path,
                      const wxString& _loc,
                      const wxString& tags,
                      const wxString& desc,
                      const wxImage& favicon)
{
    Bookmark b;
    wxString loc = _loc;
    
    // get the extension
    wxString ext;
    int ext_pos = loc.Find(wxT('.'), true);
    if (ext_pos != -1 && ext_pos > 0)
    {
        if (loc[ext_pos-1] != wxT('/'))
            ext = loc.Mid(ext_pos+1);
        if (ext.Find(L'/') != -1)
            ext = L"";
    }
  
    ext.MakeUpper();

    if (loc.Left(5).MakeLower() == wxT("http:") ||
        loc.Left(6).MakeLower() == wxT("https:") ||
        ext == wxT("HTML") ||
        ext == wxT("HTM") ||
        ext == wxT("PHP") ||
        ext == wxT("JSP") ||
        ext == wxT("ASP") ||
        ext == wxT("ASPX") ||
        ext == wxT("XML"))
    {
        // create a bookmark file
        b.setLocation(loc);
        b.setTags(tags);
        b.setDescription(desc);
        b.setFavIcon(favicon);
        b.save(path);
    }
     else if (loc.Left(7).MakeLower() == wxT("sdserv:") ||
              loc.Left(8).MakeLower() == wxT("sdservs:"))
    {
        std::wstring url = towstr(loc);

        int url_sign = url.find(L"://");
        if (url_sign == -1)
            return;

        // parse url

        std::wstring protocol;
        std::wstring server;
        std::wstring port;
        std::wstring path;
        std::wstring database;

        protocol = url.substr(0, url_sign);
        server = url.substr(url_sign+3);

        if (server.find('/') != server.npos)
        {
            path = kl::afterFirst(server, '/');
            server = kl::beforeFirst(server, '/');
        }

        if (server.find(':') != server.npos)
        {
            port = kl::afterFirst(server, ':');
            server = kl::beforeFirst(server, ':');
        }
    
        if (path.find('/') == path.npos)
        {
            database = path;
            path = '/';
        }
         else
        {
            database = kl::beforeFirst(path, '/');
            path = kl::afterFirst(path, '/');
        }


        if (path.length() == 0 || path[0] != '/')
            path = L"/" + path;

        if (port.length() == 0)
        {
            if (loc.Left(8).MakeLower() == wxT("sdservs:"))
                port = L"4820";
                 else
                port = L"4800";
        }

        // make a connection string
        std::wstring conn_str = L"xdprovider=xdclient;";
        conn_str += L"host=" + server + L";";
        conn_str += L"port=" + port + L";";
        conn_str += L"database=default;user id=admin;password=";



        // create a mount node file
        tango::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {
            db->setMountPoint(towstr(path), conn_str, path);
        }
    }
     else
    {
        if (xf_get_file_exist(towstr(loc)))
        {
            // it's a real filename, convert it to a url
            loc = filenameToUrl(loc);
        }
        
    
        // create a mount node file
        tango::IDatabasePtr db = g_app->getDatabase();
        if (db.isOk())
        {
            std::wstring cstr = L"";
            std::wstring mloc = towstr(loc);

            // is the source location itself a mount?
            std::wstring root = getMountRoot(db, mloc);
            if (root.length() > 0)
            {
                std::wstring mpath;
                if (!db->getMountPoint(root, cstr, mpath))
                    return;

                if (root == mloc)
                {
                    // mount root is an object-mount itself
                    mloc = mpath;
                }
                 else
                {
                    mloc = mloc.substr(root.length());
                }
            }


            db->setMountPoint(towstr(path), cstr, mloc);
        
            // we need to save bookmark information as well
            if (tags.Length() > 0 || desc.Length() > 0)
            {
                b.setLocation(loc);
                b.setTags(tags);
                b.setDescription(desc);
                b.save(path);
            }
        }
    }
}

bool Bookmark::load(const wxString& path)
{
    if (path.IsEmpty())
        return false;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;
    
    tango::IFileInfoPtr info = db->getFileInfo(towstr(path));
    if (info.isOk() && info->getType() == tango::filetypeFolder)
        return false;

    kl::JsonNode node = JsonConfig::loadFromDb(g_app->getDatabase(), path);
    if (!node.isOk())
        return false;

    // try to load the new format
    kl::JsonNode metadata_node = node["metadata"];
    if (metadata_node.isOk())
    {
        // check the mime type and the version
        kl::JsonNode type_node = metadata_node["type"];
        if (!type_node.isOk() || type_node.getString() != L"application/vnd.kx.bookmark")
            return false;

        kl::JsonNode version_node = metadata_node["version"];
        if (!version_node.isOk() || version_node.getInteger() != 1)
            return false;

        // if this node exists, we're working with a mount
        kl::JsonNode remote_path_node = node["remote_path"];
        if (remote_path_node.isOk())
            m_loc = towx(remote_path_node.getString());
    
        // get/create the root bookmark node
        kl::JsonNode bookmark_node = node["bookmark"];
        if (!bookmark_node.isOk())
            return false;

        kl::JsonNode location_node = bookmark_node["location"];
        if (!location_node.isOk())
            return false;

        if (!remote_path_node.isOk())
            m_loc = towx(location_node.getString());

        kl::JsonNode tags_node = bookmark_node["tags"];
        if (tags_node.isOk())
            m_tags = towx(tags_node.getString());

        kl::JsonNode description_node = bookmark_node["description"];
        if (description_node.isOk())
            m_desc = towx(description_node.getString());

        kl::JsonNode favicon_node = bookmark_node["favicon"];
        if (favicon_node.isOk())
            m_favicon = textToImage(towx(favicon_node.getString()));
             else
            m_favicon = wxImage();
        
        kl::JsonNode run_target_node = bookmark_node["run_target"];
        if (run_target_node.isOk())
            m_run_target = (run_target_node.getInteger() != 0 ? true : false);
             else
            m_run_target = false;
    
        // now we can set the path member variable
        m_path = path;

        return true;
    }


    // if we can't load the new format, try to load the old format    
    kl::JsonNode root_node = node["root"];
    if (root_node.isOk())
    {
        // if this node exists, we're working with a mount
        kl::JsonNode remote_path_node = root_node["remote_path"];
        if (remote_path_node.isOk())
            m_loc = towx(remote_path_node.getString());
    
        // get/create the root bookmark node
        kl::JsonNode bookmark_node = root_node["bookmark"];
        if (!bookmark_node.isOk())
            return false;

        kl::JsonNode location_node = bookmark_node["location"];
        if (!location_node.isOk())
            return false;

        if (!remote_path_node.isOk())
            m_loc = towx(location_node.getString());

        kl::JsonNode tags_node = bookmark_node["tags"];
        if (tags_node.isOk())
            m_tags = towx(tags_node.getString());

        kl::JsonNode description_node = bookmark_node["description"];
        if (description_node.isOk())
            m_desc = towx(description_node.getString());

        kl::JsonNode favicon_node = bookmark_node["favicon"];
        if (favicon_node.isOk())
            m_favicon = textToImage(towx(favicon_node.getString()));
             else
            m_favicon = wxImage();
        
        kl::JsonNode run_target_node = bookmark_node["run_target"];
        if (run_target_node.isOk())
            m_run_target = (run_target_node.getInteger() != 0 ? true : false);
             else
            m_run_target = false;
    
        // now we can set the path member variable
        m_path = path;

        // convert the old path to the new format
        save(path);

        return true;
    }

    // some other format that we don't know about
    return false;
}

bool Bookmark::save(const wxString& path)
{
    if (path.IsEmpty())
        return false;
    
    tango::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;
    
    tango::IFileInfoPtr info = db->getFileInfo(towstr(path));
    if (info.isOk() && info->getType() == tango::filetypeFolder)
        return false;
    

    // path exists and is different; we'll be saving to a new location
    // so delete any existing bookmark that may have been created
    if (m_path.CmpNoCase(path) != 0 && m_path.Length() > 0)
        g_app->getDatabase()->deleteFile(towstr(m_path));


    kl::JsonNode node;
    kl::JsonNode metadata_node = node["metadata"];
    metadata_node["type"] = L"application/vnd.kx.bookmark";
    metadata_node["version"] = 1;
    metadata_node["description"] = L"";


    // TODO: how do we handle saving of "remote_path"?
    // what triggers whether this should be set; seems like
    // we should think of a better way of doing this; for
    // now, don't handle it
    // node["remote_path"] = ?


    kl::JsonNode bookmark_node = node["bookmark"];
    bookmark_node["location"] = towstr(m_loc);
    bookmark_node["tags"] = towstr(m_tags);
    bookmark_node["description"] = towstr(m_desc);
    if (m_favicon.IsOk())
    {
        wxString text = imageToText(m_favicon);
        bookmark_node["favicon"] = towstr(text);
    }
    bookmark_node["run_target"].setBoolean(m_run_target);


    if (!JsonConfig::saveToDb(node, g_app->getDatabase(), path, L"application/vnd.kx.bookmark"))
        return false;


    // set the path member variable
    m_path = path;
    return true;
}


