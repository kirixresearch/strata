/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Application Client
 * Author:   David Z. Williams
 * Created:  2005-11-08
 *
 */


#ifndef __APP_FSITEMS_H
#define __APP_FSITEMS_H



xcm_interface IGenericFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IGenericFsItem")

public:

    virtual void addFsItem(IFsItemPtr item) = 0;
    virtual void deleteFsItem(IFsItemPtr item) = 0;
};


xcm_interface IFileFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IFileFsItem")

public:

    virtual void setPath(const wxString& path) = 0;
    virtual wxString getPath() = 0;
};


xcm_interface IDirectoryFsItem : public xcm::IObject
{
    XCM_INTERFACE_NAME("cfw.IDirectoryFsItem")

public:

    virtual wxString getPath() = 0;
    virtual void setPath(const wxString& path) = 0;
    virtual void setVisibleFileTypes(const wxString& file_types) = 0;
    virtual void setDefaultFileBitmap(const wxBitmap& bitmap) = 0;
    virtual void setDefaultFolderBitmap(const wxBitmap& bitmap) = 0;
    virtual void setFileTypeBitmap(const wxString& file_type, const wxBitmap& bitmap) = 0;
};


XCM_DECLARE_SMARTPTR(IGenericFsItem)
XCM_DECLARE_SMARTPTR(IFileFsItem)
XCM_DECLARE_SMARTPTR(IDirectoryFsItem)



#endif

