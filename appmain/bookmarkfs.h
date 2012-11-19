/*!
 *
 * Copyright (c) 2007-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   Benjamin I. Williams
 * Created:  2007-05-21
 *
 */


#ifndef __APP_BOOKMARKFS_H
#define __APP_BOOKMARKFS_H


// -- DbFolderFsItem class implementation --

class IEBookmarkItem : public FsItemBase
{
    XCM_CLASS_NAME("appmain.IEBookmarkItem")
    XCM_BEGIN_INTERFACE_MAP(IEBookmarkItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()

public:

    IEBookmarkItem();
    ~IEBookmarkItem();

    IFsItemEnumPtr getChildren();

    bool isDeferred() { return true; }

private:
};



#endif

