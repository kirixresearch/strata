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
#include <set>

#ifdef WIN32
#include <shlobj.h>
#endif

static std::wstring g_bookmarks_path;

static std::wstring getBookmarksLocation()
{
    if (!g_bookmarks_path.empty())
        return g_bookmarks_path;

    std::wstring path = towstr(g_app->getAppDataPath());
    path += PATH_SEPARATOR_CHAR;
    path += L"Bookmarks";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    g_bookmarks_path = path;
    return path;
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
        buf[15] = 0;
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

static std::wstring imageToText(const wxImage& image)
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
    kl::JsonNode node;
    if (!node.fromString(json))
        return false;

    if (!node.childExists("metadata"))
        return false;
    
    kl::JsonNode metadata = node["metadata"];
    if (metadata["type"].getString() != L"application/vnd.kx.application/vnd.kx.bookmark")
        return false;

    kl::JsonNode attributes = node["attributes"];

    location = attributes["location"].getString();
    tags = attributes["tags"].getString();
    description = attributes["description"].getString();
    icon = textToImage(attributes["icon"].getString());
    run_target = attributes["run_target"].getBoolean();

    return true;
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








static std::wstring getBookmarkFilePath(const std::wstring& bookmark, const std::wstring& extension = L".json")
{
    std::wstring full_path = getBookmarksLocation();
    if (full_path.empty())
        return L"";
    if (bookmark.length() == 0)
        return full_path;
    if (bookmark[0] != '/')
        full_path += PATH_SEPARATOR_STR;
    full_path += bookmark;
    full_path += extension;
    kl::replaceStr(full_path, L"/", PATH_SEPARATOR_STR);
    return full_path;
}

static std::wstring appendPaths(const std::wstring& path1, const std::wstring& path2)
{
    if (path2.empty())
        return path1;

    std::wstring res = path1;

    if (res.length() > 0 && res[res.length() - 1] != '/')
        res += L'/';

    if (path2[0] == '/')
        res += path2.substr(1);
    else
        res += path2;

    return res;
}

// BookmarkFs class implementation

IFsItemPtr BookmarkFs::getBookmarkFolderItem(const std::wstring& path)
{
    BookmarkFolder* root = new BookmarkFolder;
    root->setPath(path);

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

bool BookmarkFs::createFolder(const std::wstring& path)
{
    std::wstring full_path = getBookmarkFilePath(path, L"");
    if (full_path.empty())
        return false;

    return xf_mkdir(full_path);
}

bool BookmarkFs::loadBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarkFilePath(path);
    if (full_path.empty())
        return false;

    bool success = false;
    std::wstring json = xf_get_file_contents(full_path, &success);

    if (!success)
        return false;

    return bookmark.fromJson(json);
}

bool BookmarkFs::saveBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarkFilePath(path);
    if (full_path.empty())
        return false;

    std::wstring json = bookmark.toJson();

    return xf_put_file_contents(full_path, json);
}

bool BookmarkFs::deleteItem(const std::wstring& path)
{
    std::wstring full_path = getBookmarkFilePath(path, L"");
    if (full_path.empty())
        return false;

    if (xf_get_directory_exist(full_path))
        return xf_rmtree(full_path);

    full_path += L".json";
    return xf_remove(full_path);
}

bool BookmarkFs::moveItem(const std::wstring& old_path, const std::wstring& new_path)
{
    std::wstring p1 = getBookmarkFilePath(old_path, L"");
    std::wstring p2;
    if (p1.empty())
        return false;

    if (xf_get_directory_exist(p1))
    {
        p2 = getBookmarkFilePath(new_path, L"");
        if (p2.empty())
            return false;
 
        return xf_move(p1, p2);
    }

    p1 = getBookmarkFilePath(old_path);
    p2 = getBookmarkFilePath(new_path);

    return xf_move(p1, p2);
}


std::wstring BookmarkFs::getBookmarkItemPath(IFsItemPtr item)
{   
    IFsBookmarkItemPtr fsi = item;
    if (fsi.isNull())
        return L"";
    return fsi->getPath();
}

void BookmarkFs::setFileVisualLocation(const std::wstring& _path, int insert_index)
{
    if (insert_index < 0)
        return;

    std::wstring path = _path;
    std::wstring folder;
    std::wstring item;
    std::vector<std::wstring> order;
    std::vector<std::wstring>::iterator it;

    // find out which folder the bookmark is in
    if (path.length() > 0 && path[path.length() - 1] == '/')
        path = path.substr(0, path.length() - 1); // remove trailing slash

    if (kl::stringFrequency(path, '/') > 0)
    {
        folder = kl::beforeLast(path, '/');
        item = kl::afterLast(path, '/');
    }
     else
    {
        folder = L"/";
        item = path;
    }
    kl::makeLower(item);
    


    // STEP 1: read in existing order entries from json file
    std::wstring objorder_fname = getBookmarkFilePath(folder + L"/objorder.info", L"");

    std::wstring json;
    json = xf_get_file_contents(objorder_fname);
    if (json.length() > 0)
    {
        kl::JsonNode node;
        if (node.fromString(json) && node.childExists("order"))
        {
            kl::JsonNode o = node["order"];
            size_t i, cnt = o.getChildCount();
            for (i = 0; i < cnt; ++i)
            {
                kl::JsonNode bm = o[i];
                order.push_back(bm.getString());
            }
        }
    }



    // STEP 2: reconcile differences between folder contents on visual array
    std::set<std::wstring> existing;

    std::wstring fspath = getBookmarkFilePath(folder, L"");
    xf_dirhandle_t handle = xf_opendir(fspath);
    xf_direntry_t info;
    std::wstring full_path;

    while (xf_readdir(handle, &info))
    {
        if (info.m_name == L"." || info.m_name == L".." || info.m_name == L"objorder.info")
            continue;

        full_path = fspath;
        full_path += PATH_SEPARATOR_CHAR;
        full_path += info.m_name;

        kl::makeLower(info.m_name);

        if (info.m_name.find(L".json") != info.m_name.npos)
            existing.insert(kl::beforeLast(info.m_name, '.'));
        else if (xf_get_directory_exist(full_path))
            existing.insert(info.m_name);
    }

    xf_closedir(handle);


    // first delete any items from the order array that no longer exist
    int i;
    for (i = 0; i < (int)order.size(); ++i)
    {
        std::wstring item = order[i];
        kl::makeLower(item);
        if (existing.find(item) == existing.end())
        {
            order.erase(order.begin() + i);
            i--;
        }
    }

    // we will delete all items from |existing| that we already know about
    std::set<std::wstring>::iterator sit;
    for (it = order.begin(); it != order.end(); ++it)
    {
        sit = existing.find(*it);
        if (sit != existing.end())
            existing.erase(sit);
    }

    // now append any new items to the end
    for (sit = existing.begin(); sit != existing.end(); ++sit)
        order.push_back(*sit);



    // STEP 3: insert path in desired location


    // (first remove the item from where it was)
    for (it = order.begin(); it != order.end(); ++it)
    {
        if (*it == item)
        {
            order.erase(it);
            break;
        }
    }

    // (now insert it)
    if ((size_t)insert_index >= order.size())
        order.push_back(item);
         else
        order.insert(order.begin() + insert_index, item);



    // STEP 4: write out new json file

    kl::JsonNode node;
    kl::JsonNode o = node["order"];
    o.setArray();
    for (it = order.begin(); it != order.end(); ++it)
    {
        kl::JsonNode oi = o.appendElement();
        oi.setString(*it);
    }

    xf_put_file_contents(objorder_fname, node.toString());
}




static void reorderBookmarkEntries(const std::wstring& folder, std::vector<IFsItemPtr>& entries)
{
    // STEP 1: read in existing order entries from json file
    std::wstring objorder_fname = getBookmarkFilePath(folder + L"/objorder.info", L"");

    std::vector<std::wstring> order;
    std::vector<std::wstring>::iterator it;
    std::wstring json;
    json = xf_get_file_contents(objorder_fname);
    if (json.length() > 0)
    {
        kl::JsonNode node;
        if (node.fromString(json) && node.childExists("order"))
        {
            kl::JsonNode o = node["order"];
            size_t i, cnt = o.getChildCount();
            for (i = 0; i < cnt; ++i)
            {
                kl::JsonNode bm = o[i];
                order.push_back(bm.getString());
            }
        }
    }


    // STEP 2: index |entries|
    std::map<std::wstring, IFsItemPtr, kl::cmp_nocase> idx;
    std::map<std::wstring, IFsItemPtr, kl::cmp_nocase>::iterator mit;

    std::vector<IFsItemPtr>::iterator eit;

    for (eit = entries.begin(); eit != entries.end(); ++eit)
        idx[towstr((*eit)->getLabel())] = *eit;


    // STEP 3: create a new |entries| array in the order they appear in the |order|

    entries.clear();

    for (it = order.begin(); it != order.end(); ++it)
    {
        mit = idx.find(*it);
        if (mit == idx.end())
            continue;
        entries.push_back(mit->second);
        idx.erase(mit);
    }

    // add remaining items that weren't found in the order array
    for (mit = idx.begin(); mit != idx.end(); ++mit)
        entries.push_back(mit->second);
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
        
    fspath = getBookmarksLocation() + (fspath.length() > 0 ? PATH_SEPARATOR_STR : L"") + fspath;
        
    xf_dirhandle_t handle = xf_opendir(fspath);
    xf_direntry_t info;
    std::wstring full_path;

    while (xf_readdir(handle, &info))
    {
        if (info.m_name == L"." || info.m_name == L".." || info.m_name == L"objorder.info")
            continue;

        full_path = fspath;
        full_path += PATH_SEPARATOR_CHAR;
        full_path += info.m_name;

        if (info.m_name.find(L".json") != info.m_name.npos)
        {
            wxBitmap bmp = GETBMPSMALL(gf_document);
            std::wstring name = kl::beforeLast(info.m_name, '.');
            std::wstring bookmark_path = appendPaths(m_path, name);

            
            Bookmark b;
            if (g_app->getBookmarkFs()->loadBookmark(bookmark_path, b))
            {
                if (b.icon.IsOk())
                    bmp = wxBitmap(b.icon);
            }


            BookmarkItem* item = new BookmarkItem;
            item->setLabel(name);
            item->setBitmap(bmp);
            item->setPath(bookmark_path);
                
            IFsItemPtr f = static_cast<IFsItem*>(item);
            m_children.push_back(f);
        }
         else if (xf_get_directory_exist(full_path))
        {
            BookmarkFolder* folder = new BookmarkFolder;
            folder->setLabel(info.m_name);
            folder->setBitmap(GETBMPSMALL(gf_folder_closed));
            folder->setPath(appendPaths(m_path, info.m_name));

            IFsItemPtr f = static_cast<IFsItem*>(folder);
            m_children.push_back(f);
        }
    }

    xf_closedir(handle);

    reorderBookmarkEntries(m_path, m_children);
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







IBookmarkFsPtr createBookmarkFs()
{
    return static_cast<IBookmarkFs*>(new BookmarkFs);
}
