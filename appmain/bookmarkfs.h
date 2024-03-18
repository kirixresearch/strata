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

    Bookmark()
    {
        run_target = false;
        is_shortcut = false;
    }

public:

    std::wstring location;
    std::wstring tags;
    std::wstring description;
    bool run_target;
    bool is_shortcut;
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
    virtual bool itemExists(const std::wstring& path) = 0;

    virtual void setFileVisualLocation(const std::wstring& path, int insert_index) = 0;

    virtual std::wstring getBookmarkItemPath(IFsItemPtr item) = 0;
};
XCM_DECLARE_SMARTPTR(IBookmarkFs)

IBookmarkFsPtr createAppBookmarkFs();
IBookmarkFsPtr createProjectBookmarkFs();
IBookmarkFsPtr createProjectBookmarkFsForDatabase(xd::IDatabasePtr db);

#endif
