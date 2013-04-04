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


IFsItemPtr BookmarkFs::getRootBookmarksFolder()
{
    BookmarkFolder* root = new BookmarkFolder;
    root->setPath(L"/");

    return static_cast<IFsItem*>(root);
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
