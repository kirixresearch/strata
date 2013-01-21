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
    
    tango::INodeValuePtr file = db->openNodeFile(towstr(path));
    if (file.isNull())
        return false;
    
    // if this node exists, we're working with a mount
    tango::INodeValuePtr remotepath_node = file->getChild(L"remote_path", false);
    if (remotepath_node.isOk())
        m_loc = towx(remotepath_node->getString());
    
    // get/create the root bookmark node
    tango::INodeValuePtr bookmark_node = file->getChild(L"bookmark", false);
    if (bookmark_node.isNull())
        return false;

    tango::INodeValuePtr data;
    data = bookmark_node->getChild(L"location", false);
    if (remotepath_node.isNull())
        m_loc = towx(data->getString());
    
    data = bookmark_node->getChild(L"tags", false);
    m_tags = towx(data->getString());
    
    data = bookmark_node->getChild(L"description", false);
    m_desc = towx(data->getString());
    
    data = bookmark_node->getChild(L"favicon", false);
    if (data.isOk())
        m_favicon = textToImage(towx(data->getString()));
         else
        m_favicon = wxImage();
        
    data = bookmark_node->getChild(L"run_target", false);
    if (data.isOk())
        m_run_target = data->getBoolean();
         else
        m_run_target = false;
    
    // now we can set the path member variable
    m_path = path;
    
    return true;
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
    
    tango::INodeValuePtr file;
    if (m_path.CmpNoCase(path) == 0)
    {
        // edit the existing node file
        file = db->openNodeFile(towstr(path));
        if (file.isNull())
            return false;
    }
     else
    {
        if (m_path.Length() > 0)
        {
            // move the existing node file
            if (!db->moveFile(towstr(m_path), towstr(path)))
                return false;
            
            // open the node file from its new location
            file = db->openNodeFile(towstr(path));
            if (file.isNull())
                return false;
        }
         else
        {
            // create a new node file
            file = db->createNodeFile(towstr(path));
            if (file.isNull())
                return false;
        }
    }
    
    // if this node exists, we're working with a mount
    // and need to fill out this information
    tango::INodeValuePtr remotepath_node = file->getChild(L"remote_path", false);
    if (remotepath_node.isOk())
        remotepath_node->setString(towstr(m_loc));

    // get/create the root bookmark node
    tango::INodeValuePtr bookmark_node = file->getChild(L"bookmark", true);
    if (bookmark_node.isNull())
        return false;
    
    tango::INodeValuePtr data;
    data = bookmark_node->getChild(L"location", true);
    data->setString(towstr(m_loc));
    
    data = bookmark_node->getChild(L"tags", true);
    data->setString(towstr(m_tags));
    
    data = bookmark_node->getChild(L"description", true);
    data->setString(towstr(m_desc));
    
    if (m_favicon.IsOk())
    {
        data = bookmark_node->getChild(L"favicon", true);
        wxString text = imageToText(m_favicon);
        data->setString(towstr(text));
    }
    
    data = bookmark_node->getChild(L"run_target", true);
    data->setBoolean(m_run_target);
    
    // now we can set the path member variable
    m_path = path;
    
    return true;
}


