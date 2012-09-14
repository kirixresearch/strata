/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client Framework
 * Author:   David Z. Williams
 * Created:  2005-11-09
 *
 */


#ifndef __CFW_FSITEMS_PRIVATE_H
#define __CFW_FSITEMS_PRIVATE_H


#include <map>
#include "fsitems.h"


namespace cfw
{


class GenericFsItem : public FsItemBase,
                      public IGenericFsItem
{
    XCM_CLASS_NAME("cfw.GenericFsItem")
    XCM_BEGIN_INTERFACE_MAP(GenericFsItem)
        XCM_INTERFACE_ENTRY(IGenericFsItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()
private:

    bool isDeferred() { return false; }
    bool hasChildren() { return m_fsitems.size() > 0 ? true : false; }
    void addFsItem(IFsItemPtr item);
    void deleteFsItem(IFsItemPtr item);
    IFsItemEnumPtr getChildren();

private:

    std::vector<IFsItemPtr> m_fsitems;

};


class DirectoryFsItem : public FsItemBase,
                        public IDirectoryFsItem
{
    XCM_CLASS_NAME("cfw.DirectoryFsItem")
    XCM_BEGIN_INTERFACE_MAP(DirectoryFsItem)
        XCM_INTERFACE_ENTRY(IDirectoryFsItem)
        XCM_INTERFACE_CHAIN(FsItemBase)
    XCM_END_INTERFACE_MAP()


public:
    DirectoryFsItem();
    ~DirectoryFsItem();

    wxString getLabel();
    
    bool isDeferred();
    bool hasChildren();

    wxString getPath();
    void setPath(const wxString& path);
    void setVisibleFileTypes(const wxString& file_types);
    void setDefaultFileBitmap(const wxBitmap& bitmap);
    void setDefaultFolderBitmap(const wxBitmap& bitmap);
    void setFileTypeBitmap(const wxString& file_type, const wxBitmap& bitmap);

    // FsItem derived functions

    IFsItemEnumPtr getChildren();
    
private:
    wxString m_path;
    wxString m_file_types;

    std::map<wxString, wxBitmap> m_file_type_bitmaps;
    wxBitmap m_default_file_bitmap;
    wxBitmap m_default_folder_bitmap;
};


};  // namespace cfw


#endif


