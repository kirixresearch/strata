/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-05-21
 *
 */


#ifndef __APP_BOOKMARKFS_H
#define __APP_BOOKMARKFS_H


class Bookmark
{
public:

    Bookmark();
    bool fromJson(const std::wstring& json);
    std::wstring toJson();

public:

    std::wstring location;
    std::wstring tags;
    std::wstring description;
    bool run_target;
    wxImage icon;
};

class BookmarkFs
{
public:

    static IFsItemPtr getRootBookmarksFolder();

    static bool createBookmark(const std::wstring& path,
                               const std::wstring& location,
                               const std::wstring& tags = L"",
                               const std::wstring& description = L"",
                               const wxImage& icon = wxImage());
    static bool createFolder(const std::wstring& path);
    static bool loadBookmark(const std::wstring& path, Bookmark& bookmark);
    static bool saveBookmark(const std::wstring& path, Bookmark& bookmark);
    static bool deleteItem(const std::wstring& path);

    static void setFileVisualLocation(const std::wstring& path, int insert_index);
};



class BookmarkFolder : public FsItemBase
{
    XCM_CLASS_NAME("appmain.BookmarkFolder")
    XCM_BEGIN_INTERFACE_MAP(BookmarkFolder)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    BookmarkFolder();
    ~BookmarkFolder();

    IFsItemEnumPtr getChildren();

    bool isDeferred() { return false; }
    bool isFolder() { return true; }
    void setPath(const std::wstring& path) { m_path = path; }
    bool hasChildren()
    {
        if (!m_populated)
            populate();
        return m_children.size() > 0 ? true : false;
    }

private:

    void populate();

private:

    bool m_populated;
    std::wstring m_path;
    std::vector<IFsItemPtr> m_children;
};


class BookmarkItem : public FsItemBase
{
    XCM_CLASS_NAME("appmain.BookmarkItem")
    XCM_BEGIN_INTERFACE_MAP(BookmarkItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    BookmarkItem();
    ~BookmarkItem();

    bool isDeferred() { return false; }
};



#endif

