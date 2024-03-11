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
#include "dbdoc.h"
#include <wx/stdpaths.h>
#include <wx/sstream.h>
#include <wx/buffer.h>
#include <wx/mstream.h>
#include <set>


#ifdef WIN32
#include <shlobj.h>
#endif


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

    return (b1 * 16) + b2;
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

static bool jsonToBookmark(const std::wstring& json, Bookmark& bookmark)
{
    kl::JsonNode node;
    if (!node.fromString(json))
        return false;

    if (node.childExists("root"))
    {
        // old 4.5.4 compatible format
        kl::JsonNode root = node["root"];
        if (!root.childExists("bookmark"))
            return false;
        
        kl::JsonNode bookmark_node = root["bookmark"];
        bookmark.location = bookmark_node["location"].getString();
        bookmark.tags = bookmark_node["tags"].getString();
        bookmark.description = bookmark_node["description"].getString();
        bookmark.run_target = bookmark_node["run_target"].getBoolean();
        return true;
    }

    if (!node.childExists("metadata"))
        return false;

    kl::JsonNode metadata = node["metadata"];
    if (metadata["type"].getString() != L"application/vnd.kx.application/vnd.kx.bookmark")
        return false;

    kl::JsonNode attributes = node["attributes"];

    bookmark.location = attributes["location"].getString();
    bookmark.tags = attributes["tags"].getString();
    bookmark.description = attributes["description"].getString();
    bookmark.icon = textToImage(attributes["icon"].getString());
    bookmark.run_target = attributes["run_target"].getBoolean();

    return true;
}

static std::wstring bookmarkToJson(Bookmark& bookmark)
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

    node["attributes"]["location"] = bookmark.location;
    node["attributes"]["tags"] = bookmark.tags;
    node["attributes"]["description"] = bookmark.description;
    node["attributes"]["icon"] = imageToText(bookmark.icon);
    node["attributes"]["run_target"].setBoolean(bookmark.run_target);

    return node.toString();
}







xcm_interface IFsBookmarkItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IFsBookmarkItem")

public:

    virtual const std::wstring & getPath() = 0;
};
XCM_DECLARE_SMARTPTR(IFsBookmarkItem)



xcm_interface IBookmarkFsPrivate : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IBookmarkFsPrivate")

public:

    virtual std::vector<IFsItemPtr> getBookmarkFolderItems(const std::wstring& path) = 0;
};
XCM_DECLARE_SMARTPTR(IBookmarkFsPrivate)





class BookmarkFolder : public FsItemBase, public IFsBookmarkItem
{
    XCM_CLASS_NAME("appmain.BookmarkFolder")
        XCM_BEGIN_INTERFACE_MAP(BookmarkFolder)
        XCM_INTERFACE_ENTRY(IFsBookmarkItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    BookmarkFolder()
    {
        m_populated = false;
    }

    ~BookmarkFolder()
    {
    }

    IFsItemEnumPtr getChildren()
    {
        if (!m_populated)
            populate();

        xcm::IVectorImpl<IFsItemPtr>* v = new xcm::IVectorImpl<IFsItemPtr>;
        v->setVector(m_children);
        return v;
    }


    bool isDeferred() { return false; }
    bool isFolder() { return true; }
    void setPath(const std::wstring& path) { m_path = path; }
    const std::wstring& getPath() { return m_path; }
    bool hasChildren()
    {
        if (!m_populated)
            populate();
        return m_children.size() > 0 ? true : false;
    }

private:

    void populate()
    {
        if (!m_populated)
        {
            IBookmarkFsPtr fs = g_app->getBookmarkFs();
            IBookmarkFsPrivatePtr priv = fs;

            m_children = priv->getBookmarkFolderItems(m_path);

            m_populated = true;
        }
    }

private:

    bool m_populated;
    std::wstring m_path;
    std::vector<IFsItemPtr> m_children;
};


class BookmarkItem : public FsItemBase, public IFsBookmarkItem
{
    XCM_CLASS_NAME("appmain.BookmarkItem")
        XCM_BEGIN_INTERFACE_MAP(BookmarkItem)
        XCM_INTERFACE_ENTRY(IFsBookmarkItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    BookmarkItem() { }
    ~BookmarkItem() { }

    bool isDeferred() { return false; }
    void setPath(const std::wstring& path) { m_path = path; }
    const std::wstring& getPath() { return m_path; }

private:

    std::wstring m_path;
};










// AppBookmarkFs class implementation


class AppBookmarkFs : public IBookmarkFs,
                      public IBookmarkFsPrivate
{
    XCM_CLASS_NAME("appmain.AppBookmarkFs")
        XCM_BEGIN_INTERFACE_MAP(AppBookmarkFs)
        XCM_INTERFACE_ENTRY(IBookmarkFs)
        XCM_INTERFACE_ENTRY(IBookmarkFsPrivate)
    XCM_END_INTERFACE_MAP()

public:

    IFsItemPtr getBookmarkFolderItem(const std::wstring& path);

    bool createBookmark(const std::wstring& path,
        const std::wstring& location,
        const std::wstring& tags = L"",
        const std::wstring& description = L"",
        const wxImage& icon = wxImage());
    bool createFolder(const std::wstring& path);
    bool loadBookmark(const std::wstring& path, Bookmark& bookmark);
    bool saveBookmark(const std::wstring& path, Bookmark& bookmark);
    bool deleteItem(const std::wstring& path);
    bool moveItem(const std::wstring& old_path, const std::wstring& new_path);

    void setFileVisualLocation(const std::wstring& path, int insert_index);

    std::wstring getBookmarkItemPath(IFsItemPtr item);

private:

    std::vector<IFsItemPtr> getBookmarkFolderItems(const std::wstring& path);
    std::wstring getBookmarksLocation();
    std::wstring getBookmarkFilePath(const std::wstring& bookmark, const std::wstring& extension = L".json");

    void loadOrder(const std::wstring& folder, std::vector<std::wstring>& order);
    void saveOrder(const std::wstring& folder, const std::vector<std::wstring>& order);
    void reorderBookmarkEntries(const std::wstring& folder, std::vector<IFsItemPtr>& entries);

private:

    std::wstring m_bookmarks_path;
};


IFsItemPtr AppBookmarkFs::getBookmarkFolderItem(const std::wstring& path)
{
    BookmarkFolder* root = new BookmarkFolder;
    root->setPath(path);

    return static_cast<IFsItem*>(root);
}


bool AppBookmarkFs::createBookmark(const std::wstring& path,
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

bool AppBookmarkFs::createFolder(const std::wstring& path)
{
    std::wstring full_path = getBookmarkFilePath(path, L"");
    if (full_path.empty())
        return false;

    return xf_mkdir(full_path);
}

bool AppBookmarkFs::loadBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarkFilePath(path);
    if (full_path.empty())
        return false;

    bool success = false;
    std::wstring json = xf_get_file_contents(full_path, &success);

    if (!success)
        return false;

    return jsonToBookmark(json, bookmark);
}

bool AppBookmarkFs::saveBookmark(const std::wstring& path, Bookmark& bookmark)
{
    std::wstring full_path = getBookmarkFilePath(path);
    if (full_path.empty())
        return false;

    std::wstring json = bookmarkToJson(bookmark);

    return xf_put_file_contents(full_path, json);
}

bool AppBookmarkFs::deleteItem(const std::wstring& path)
{
    std::wstring full_path = getBookmarkFilePath(path, L"");
    if (full_path.empty())
        return false;

    if (xf_get_directory_exist(full_path))
        return xf_rmtree(full_path);

    full_path += L".json";
    return xf_remove(full_path);
}

bool AppBookmarkFs::moveItem(const std::wstring& old_path, const std::wstring& new_path)
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


std::wstring AppBookmarkFs::getBookmarkItemPath(IFsItemPtr item)
{   
    IFsBookmarkItemPtr fsi = item;
    if (fsi.isNull())
        return L"";
    return fsi->getPath();
}

void AppBookmarkFs::loadOrder(const std::wstring& folder, std::vector<std::wstring>& order)
{
    order.clear();

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

}

void  AppBookmarkFs::saveOrder(const std::wstring& folder, const std::vector<std::wstring>& order)
{
    std::wstring objorder_fname = getBookmarkFilePath(folder + L"/objorder.info", L"");
    std::vector<std::wstring>::const_iterator it;

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


void AppBookmarkFs::setFileVisualLocation(const std::wstring& _path, int insert_index)
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
    loadOrder(folder, order);

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
    saveOrder(folder, order);
}




void AppBookmarkFs::reorderBookmarkEntries(const std::wstring& folder, std::vector<IFsItemPtr>& entries)
{
    // STEP 1: read in existing order entries from json file
    std::vector<std::wstring> order;
    std::vector<std::wstring>::iterator it;
    std::wstring json;

    loadOrder(folder, order);

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



std::vector<IFsItemPtr> AppBookmarkFs::getBookmarkFolderItems(const std::wstring& folder_path)
{
    std::vector<IFsItemPtr> result_items;

    std::wstring fspath = folder_path;
    if (fspath.length() > 0 && fspath[0] == '/')
        fspath.erase(0, 1);

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
            std::wstring bookmark_path = appendPaths(folder_path, name);


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
            result_items.push_back(f);
        }
        else if (xf_get_directory_exist(full_path))
        {
            BookmarkFolder* folder = new BookmarkFolder;
            folder->setLabel(info.m_name);
            folder->setBitmap(GETBMPSMALL(gf_folder_closed));
            folder->setPath(appendPaths(folder_path, info.m_name));

            IFsItemPtr f = static_cast<IFsItem*>(folder);
            result_items.push_back(f);
        }
    }

    xf_closedir(handle);

    reorderBookmarkEntries(folder_path, result_items);

    return result_items;
}



std::wstring AppBookmarkFs::getBookmarksLocation()
{
    if (!m_bookmarks_path.empty())
        return m_bookmarks_path;

    std::wstring path = towstr(g_app->getAppDataPath());
    path += PATH_SEPARATOR_CHAR;
    path += L"Bookmarks";

    if (!xf_get_directory_exist(path))
    {
        if (!xf_mkdir(path))
            return L"";
    }

    m_bookmarks_path = path;
    return path;
}


std::wstring AppBookmarkFs::getBookmarkFilePath(const std::wstring& bookmark, const std::wstring& extension)
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









// ProjectBookmarkFs class implementation

class ProjectBookmarkFs : public IBookmarkFs,
                          public IBookmarkFsPrivate
{
    XCM_CLASS_NAME("appmain.ProjectBookmarkFs")
        XCM_BEGIN_INTERFACE_MAP(ProjectBookmarkFs)
        XCM_INTERFACE_ENTRY(IBookmarkFs)
        XCM_INTERFACE_ENTRY(IBookmarkFsPrivate)
    XCM_END_INTERFACE_MAP()

public:

    IFsItemPtr getBookmarkFolderItem(const std::wstring& path);

    bool createBookmark(const std::wstring& path,
        const std::wstring& location,
        const std::wstring& tags = L"",
        const std::wstring& description = L"",
        const wxImage& icon = wxImage());
    bool createFolder(const std::wstring& path);
    bool loadBookmark(const std::wstring& path, Bookmark& bookmark);
    bool saveBookmark(const std::wstring& path, Bookmark& bookmark);
    bool deleteItem(const std::wstring& path);
    bool moveItem(const std::wstring& old_path, const std::wstring& new_path);

    void setFileVisualLocation(const std::wstring& path, int insert_index);

    std::wstring getBookmarkItemPath(IFsItemPtr item);

private:


    std::vector<IFsItemPtr> getBookmarkFolderItems(const std::wstring& path);
    std::wstring getBookmarkFilePath(const std::wstring& bookmark);
    std::wstring getProjectBookmarkFolder();

    void reorderBookmarkEntries(const std::wstring& folder, std::vector<IFsItemPtr>& entries);
    void loadOrder(const std::wstring& folder, std::vector<std::wstring>& order);
    void saveOrder(const std::wstring& folder, const std::vector<std::wstring>& order);

private:

    std::wstring m_bookmarks_path;
};


IFsItemPtr ProjectBookmarkFs::getBookmarkFolderItem(const std::wstring& path)
{
    BookmarkFolder* root = new BookmarkFolder;
    root->setPath(path);

    return static_cast<IFsItem*>(root);
}


bool ProjectBookmarkFs::createBookmark(const std::wstring& path,
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

bool ProjectBookmarkFs::createFolder(const std::wstring& _path)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, _path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    return db->createFolder(path);
}

bool ProjectBookmarkFs::loadBookmark(const std::wstring& _path, Bookmark& bookmark)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, _path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    xd::IFileInfoPtr finfo = db->getFileInfo(path);
    if (finfo->isMount())
    {
        std::wstring cstr, rpath;
        if (db->getMountPoint(path, cstr, rpath))
        {
            bookmark.location = path;
            bookmark.tags = L"";
            bookmark.description = L"";
            bookmark.icon = wxImage();
            bookmark.run_target = false;

            if (cstr.empty() && rpath.length() > 0)
            {
                bookmark.location = rpath;
                bookmark.is_shortcut = true;
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        std::wstring contents;
        if (!readStreamTextFile(db, path, contents))
            return false;

        return jsonToBookmark(contents, bookmark);
    }

}

bool ProjectBookmarkFs::saveBookmark(const std::wstring& _path, Bookmark& bookmark)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, _path);

    std::wstring json = bookmarkToJson(bookmark);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    db->deleteFile(path); // delete the old node file -- having a node file (.xml) and a json file by the same name will freeze 4.5.4

    return writeStreamTextFile(db, path, json);
}

bool ProjectBookmarkFs::deleteItem(const std::wstring& _path)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, _path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    return db->deleteFile(path);
}

bool ProjectBookmarkFs::moveItem(const std::wstring& _old_path, const std::wstring& _new_path)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring old_path = appendPaths(base_path, _old_path);
    std::wstring new_path = appendPaths(base_path, _new_path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return false;

    return db->moveFile(old_path, new_path);
}


std::wstring ProjectBookmarkFs::getBookmarkItemPath(IFsItemPtr item)
{
    IFsBookmarkItemPtr fsi = item;
    if (fsi.isNull())
        return L"";
    return fsi->getPath();
}

void ProjectBookmarkFs::loadOrder(const std::wstring& folder, std::vector<std::wstring>& order)
{
    order.clear();

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(appendPaths(base_path, folder), L".objorder2");

    std::wstring contents;
    if (readStreamTextFile(db, path, contents))
    {
        kl::JsonNode node;
        if (node.fromString(contents) && node.childExists("order"))
        {
            kl::JsonNode o = node["order"];
            size_t i, cnt = o.getChildCount();
            for (i = 0; i < cnt; ++i)
            {
                kl::JsonNode bm = o[i];
                order.push_back(bm.getString());
            }
        }

        return;
    }

    path = appendPaths(appendPaths(base_path, folder), L".objorder");
    if (readStreamTextFile(db, path, contents))
    {
        kl::JsonNode node;
        if (node.fromString(contents) && node.childExists("root") && node["root"].childExists("positions"))
        {
            kl::JsonNode o = node["root"]["positions"];

            int i = 0;
            std::wstring node_name;
            while (true)
            {
                node_name = kl::stdswprintf(L"position%d", i);
                if (o.childExists(node_name) && o[node_name].childExists("name"))
                {
                    std::wstring name = o[node_name]["name"].getString();
                    if (name.substr(0, 9) != L".objorder")
                    {
                        order.push_back(name);
                    }

                    ++i;
                }
                else
                {
                    break;
                }
            }

        }
    }
}


void ProjectBookmarkFs::saveOrder(const std::wstring& folder, const std::vector<std::wstring>& order)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(appendPaths(base_path, folder), L".objorder2");

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
        return;

    std::vector<std::wstring>::const_iterator it;

    kl::JsonNode node;
    kl::JsonNode o = node["order"];
    o.setArray();
    for (it = order.begin(); it != order.end(); ++it)
    {
        kl::JsonNode oi = o.appendElement();
        oi.setString(*it);
    }

    writeStreamTextFile(db, path, node.toString(), L"text/json");
}

void ProjectBookmarkFs::setFileVisualLocation(const std::wstring& _path, int insert_index)
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
        if (folder.empty())
            folder = L"/";
        item = kl::afterLast(path, '/');
    }
    else
    {
        folder = L"/";
        item = path;
    }


    // STEP 1: read in existing order entries from json file
    loadOrder(folder, order);

    // STEP 2: reconcile differences between folder contents on visual array
    std::set<std::wstring> existing;


    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring db_path = appendPaths(base_path, folder);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isOk())
    {
        xd::IFileInfoEnumPtr files = db->getFolderInfo(db_path);
        size_t file_count = files->size();
        for (size_t i = 0; i < file_count; ++i)
        {
            xd::IFileInfoPtr file = files->getItem(i);
            std::wstring name = file->getName();

            if (name.substr(0, 1) == L".")
                continue;

            kl::makeLower(name);
            existing.insert(name);
        }
    }


    // first delete any items from the order array that no longer exist
    int i;
    for (i = 0; i < (int)order.size(); ++i)
    {
        std::wstring s = order[i];
        kl::makeLower(s);
        if (existing.find(s) == existing.end())
        {
            order.erase(order.begin() + i);
            i--;
        }
    }

    // we will delete all items from |existing| that we already know about
    std::set<std::wstring>::iterator sit;
    for (it = order.begin(); it != order.end(); ++it)
    {
        std::wstring s = *it;
        kl::makeLower(s);

        sit = existing.find(s);
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
    saveOrder(folder, order);
}




void ProjectBookmarkFs::reorderBookmarkEntries(const std::wstring& folder, std::vector<IFsItemPtr>& entries)
{
    // STEP 1: read in existing order entries from json file
    std::vector<std::wstring> order;
    std::vector<std::wstring>::iterator it;
    std::wstring json;

    loadOrder(folder, order);

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


std::wstring ProjectBookmarkFs::getProjectBookmarkFolder()
{
    xd::IDatabasePtr database = g_app->getDatabase();
    if (database.isNull())
        return L"";

    std::wstring res;

    res = L"/.appdata/";
    res += database->getActiveUid();
    res += L"/bookmarks";

    if (!database->getFileExist(res))
    {
        database->createFolder(res);
    }

    return res;
}


std::vector<IFsItemPtr> ProjectBookmarkFs::getBookmarkFolderItems(const std::wstring& folder_path)
{
    std::vector<IFsItemPtr> result_items;

    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, folder_path);

    xd::IDatabasePtr db = g_app->getDatabase();
    if (db.isNull())
    {
        return result_items;
    }

    DbDoc* dbdoc = g_app->getDbDoc();
    if (!dbdoc)
    {
        return result_items;
    }


    xd::IFileInfoEnumPtr files = db->getFolderInfo(path);
    size_t file_count = files->size();
    for (size_t i = 0; i < file_count; ++i)
    {
        xd::IFileInfoPtr file = files->getItem(i);

        if (file->getName().substr(0,1) == L".")
            continue;

        if (file->getType() == xd::filetypeFolder)
        {
            BookmarkFolder* folder = new BookmarkFolder();
            folder->setLabel(file->getName());
            folder->setBitmap(GETBMPSMALL(gf_folder_closed));
            folder->setPath(appendPaths(folder_path, file->getName()));

            result_items.push_back(folder);
        }
        else
        {
            BookmarkItem* item = new BookmarkItem();
            item->setLabel(file->getName());
            item->setPath(appendPaths(folder_path, file->getName()));
            item->setBitmap(GETBMPSMALL(gf_document));

            result_items.push_back(item);
        }
    }

    reorderBookmarkEntries(folder_path, result_items);

    return result_items;
}


std::wstring ProjectBookmarkFs::getBookmarkFilePath(const std::wstring& bookmark)
{
    std::wstring base_path = getProjectBookmarkFolder();
    std::wstring path = appendPaths(base_path, bookmark);
    return bookmark;
}


















IBookmarkFsPtr createAppBookmarkFs()
{
    return static_cast<IBookmarkFs*>(new AppBookmarkFs);
}

IBookmarkFsPtr createProjectBookmarkFs()
{
    return static_cast<IBookmarkFs*>(new ProjectBookmarkFs);
}
