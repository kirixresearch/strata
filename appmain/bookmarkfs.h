/*!
 *
 * Copyright (c) 2007-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-05-21
 *
 */


#ifndef H_APP_BOOKMARKFS_H
#define H_APP_BOOKMARKFS_H


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


xcm_interface IBookmarkFs : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IBookmarkFs")

public:

    virtual IFsItemPtr getBookmarkFolderItem(const std::wstring& path) = 0;

    virtual bool createBookmark(const std::wstring& path,
                                const std::wstring& location,
                                const std::wstring& tags = L"",
                                const std::wstring& description = L"",
                                const wxImage& icon = wxImage()) = 0;
    virtual bool createFolder(const std::wstring& path) = 0;
    virtual bool loadBookmark(const std::wstring& path, Bookmark& bookmark) = 0;
    virtual bool saveBookmark(const std::wstring& path, Bookmark& bookmark) = 0;
    virtual bool deleteItem(const std::wstring& path) = 0;
    virtual bool moveItem(const std::wstring& old_path, const std::wstring& new_path) = 0;

    virtual void setFileVisualLocation(const std::wstring& path, int insert_index) = 0;

    virtual std::wstring getBookmarkItemPath(IFsItemPtr item) = 0;
};
XCM_DECLARE_SMARTPTR(IBookmarkFs)


xcm_interface IFsBookmarkItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("appmain.IFsBookmarkItem")

public:

    virtual const std::wstring& getPath() = 0;
};
XCM_DECLARE_SMARTPTR(IFsBookmarkItem)


class BookmarkFs : public IBookmarkFs
{
    XCM_CLASS_NAME("appmain.BookmarkFs")
        XCM_BEGIN_INTERFACE_MAP(BookmarkFs)
        XCM_INTERFACE_ENTRY(IBookmarkFs)
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
};


class BookmarkFolder : public FsItemBase, public IFsBookmarkItem
{
    XCM_CLASS_NAME("appmain.BookmarkFolder")
    XCM_BEGIN_INTERFACE_MAP(BookmarkFolder)
        XCM_INTERFACE_ENTRY(IFsBookmarkItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    BookmarkFolder();
    ~BookmarkFolder();

    IFsItemEnumPtr getChildren();

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

    void populate();

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

    BookmarkItem();
    ~BookmarkItem();

    bool isDeferred() { return false; }
    void setPath(const std::wstring& path) { m_path = path; }
    const std::wstring& getPath() { return m_path; }

private:

    std::wstring m_path;
};



IBookmarkFsPtr createBookmarkFs();


#endif

