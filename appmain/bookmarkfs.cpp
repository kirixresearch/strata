/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-05-21
 *
 */


#include "appmain.h"
#include "bookmarkfs.h"
#include <wx/stdpaths.h>
#include <wx/sstream.h>
#include <wx/buffer.h>
#include <wx/mstream.h>

#ifdef WIN32
#include <shlobj.h>
#endif


static std::wstring getBookmarksLocation()
{
#ifdef WIN32
    TCHAR tpath[MAX_PATH];
    if (S_OK != SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, tpath))
        return L"";

    std::wstring path = kl::towstring(tpath);
    path += L"\\";
    path += kl::towstring(APP_CONFIG_KEY);

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    path += L"\\";
    path += kl::towstring(APP_COMPANY_KEY);

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    path += L"\\";
    path += L"Bookmarks";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    return path;
#else

    std::wstring path = towstr(wxStandardPaths::Get().GetDocumentsDir());
    path += L"/.";
    path += kl::towstring(APP_CONFIG_KEY);

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path)) // TODO: add directory mode
            return L"";
    }

    path += L"/";
    path += kl::towstring(APP_COMPANY_KEY);

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    path += L"/";
    path += L"Bookmarks";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    return path;

#endif
}


static void bin2hex(const unsigned char* data, size_t len, std::wstring& output)
{
    output.reserve((len*2) + 1);
    output = L"X"; // version
    
    std::wstring s;
    wchar_t buf[16];
    for (size_t i = 0; i < len; ++i)
    {
        swprintf(buf, 16, L"%02X", data[i]);
        output += buf;
    }
}


inline unsigned char hex2byte(unsigned char b1, unsigned char b2)
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

static void hex2bin(const std::wstring& _input, wxMemoryBuffer& output)
{
    std::wstring input = _input;
    kl::trim(input);
    
    output.SetBufSize(0);
    
    if (input.length() <= 1)
        return;
    
    wxChar ch1, ch2;
    const wchar_t* buf = input.c_str();
    
    if (*buf != 'X') // check version
        return;
    buf++;
    
    output.SetBufSize((input.length()-1)/2);
    
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


static wxImage textToImage(const std::wstring& str)
{
    if (str.empty())
        return wxImage();

    wxMemoryBuffer buf;
    hex2bin(str, buf);
    
    wxMemoryInputStream input_stream(buf.GetData(), buf.GetDataLen());
    
    wxImage img;
    if (buf.GetDataLen() == 0)
        return img;
        
    img.LoadFile(input_stream, wxBITMAP_TYPE_PNG);
    return img;
}

static std::wstring imageToText(wxImage& image)
{
    if (!image.IsOk())
        return L"";

    wxMemoryOutputStream output_stream;
    image.SaveFile(output_stream, wxBITMAP_TYPE_PNG);
    
    size_t len = output_stream.GetLength();
    wxMemoryBuffer buf;
    output_stream.CopyTo(buf.GetAppendBuf(len), len);
    buf.UngetAppendBuf(len);
    
    std::wstring ret;
    bin2hex((const unsigned char*)buf.GetData(), buf.GetDataLen(), ret);
    return ret;
}

// Bookmark class implementation


Bookmark::Bookmark()
{
    run_target = false;
}

bool Bookmark::fromJson(const std::wstring& json)
{
    return false;
}

std::wstring Bookmark::toJson()
{
/*
    // example format:    
    {
        "metadata":
        {
            "type" : "application/vnd.kx.bookmark",
            "version" : 1,
            "description" : ""
        },
        "attributes":
        {
            "location": <location>,
            "tags": <tags>,
            "description": <description>,
            "icon": <icon>,
            "run_target": false

        }
    }
*/

    kl::JsonNode node;
    node["metadata"]["type"] = L"application/vnd.kx.application/vnd.kx.bookmark";
    node["metadata"]["version"] = 1;
    node["metadata"]["description"] = L"";

    node["attributes"]["location"] = location;
    node["attributes"]["tags"] = tags;
    node["attributes"]["description"] = description;
    node["attributes"]["icon"] = imageToText(icon);
    node["attributes"]["run_target"].setBoolean(run_target);

    return node.toString();
}










// BookmarkFs class implementation

IFsItemPtr BookmarkFs::getRootBookmarksFolder()
{
    BookmarkFolder* root = new BookmarkFolder;
    root->setPath(L"/");

    return static_cast<IFsItem*>(root);
}


bool BookmarkFs::createBookmark(const std::wstring& path,
                                const std::wstring& location,
                                const std::wstring& tags,
                                const std::wstring& description,
                                const wxImage& icon)
{
    Bookmark b;
    b.location = location;
    b.tags = tags;
    b.description = description;
    b.icon = icon;

    return saveBookmark(path, b);
}

bool BookmarkFs::loadBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarksLocation();
    if (full_path.empty())
        return false;
    full_path += PATH_SEPARATOR_STR;
    full_path += path;
    full_path += L".json";
    kl::replaceStr(full_path, L"/", PATH_SEPARATOR_STR);

    bool success = false;
    std::wstring json = xf_get_file_contents(full_path, &success);

    if (!success)
        return false;

    return bookmark.fromJson(json);

}

bool BookmarkFs::saveBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarksLocation();
    if (full_path.empty())
        return false;
    full_path += PATH_SEPARATOR_STR;
    full_path += path;
    full_path += L".json";
    kl::replaceStr(full_path, L"/", PATH_SEPARATOR_STR);

    std::wstring json = bookmark.toJson();

    return xf_put_file_contents(full_path, json);
}

bool BookmarkFs::deleteItem(const std::wstring& path)
{
    std::wstring full_path = getBookmarksLocation();
    if (full_path.empty())
        return false;
    full_path += PATH_SEPARATOR_STR;
    full_path += path;
    kl::replaceStr(full_path, L"/", PATH_SEPARATOR_STR);

    if (xf_get_directory_exist(full_path))
        return xf_rmtree(full_path);

    full_path += L".json";
    return xf_remove(full_path);
}



BookmarkFolder::BookmarkFolder()
{
    m_populated = false;
}

BookmarkFolder::~BookmarkFolder()
{
}

void BookmarkFolder::populate()
{
    m_populated = true;

    std::wstring fspath = m_path;
    if (fspath.length() > 0 && fspath[0] == '/')
        fspath.erase(0,1);
        
    fspath = getBookmarksLocation() + (fspath.length()>0?PATH_SEPARATOR_STR:L"") + fspath;
        
    xf_dirhandle_t handle = xf_opendir(fspath);
    xf_direntry_t info;
    std::wstring full_path;

    while (xf_readdir(handle, &info))
    {
        if (info.m_name == L"." || info.m_name == L"..")
            continue;

        full_path = fspath;
        full_path += PATH_SEPARATOR_CHAR;
        full_path += info.m_name;

        if (info.m_name.find(L".json") != info.m_name.npos)
        {
            BookmarkItem* item = new BookmarkItem;
            item->setLabel(towx(kl::beforeLast(info.m_name, '.')));
            item->setBitmap(GETBMP(gf_document_16));
                
            IFsItemPtr f = static_cast<IFsItem*>(item);
            m_children.push_back(f);
        }
         else if (xf_get_directory_exist(full_path))
        {
            BookmarkFolder* folder = new BookmarkFolder;
            folder->setLabel(info.m_name);
            folder->setBitmap(GETBMP(gf_folder_closed_16));
            folder->setPath(m_path + L"/" + info.m_name);

            IFsItemPtr f = static_cast<IFsItem*>(folder);
            m_children.push_back(f);
        }
    }

    xf_closedir(handle);
}

IFsItemEnumPtr BookmarkFolder::getChildren()
{
    if (!m_populated)
        populate();

    xcm::IVectorImpl<IFsItemPtr>* v = new xcm::IVectorImpl<IFsItemPtr>;
    v->setVector(m_children);
    return v;
}










BookmarkItem::BookmarkItem()
{
}

BookmarkItem::~BookmarkItem()
{
}
