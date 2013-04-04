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



class BookmarkFs
{
public:

    static IFsItemPtr getRootBookmarksFolder();
    static bool createBookmark(const wxString& path,
                               const wxString& loc,
                               const wxString& tags = L"",
                               const wxString& desc = L"",
                               const wxImage& favicon = wxImage());
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

