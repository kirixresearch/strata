/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-13
 *
 */


#ifndef __XDCOMMON_FILEINFO_H
#define __XDCOMMON_FILEINFO_H


namespace xdcommon
{


class FileInfo : public xd::IFileInfo
{
    XCM_CLASS_NAME("xd.FileInfo")
    XCM_BEGIN_INTERFACE_MAP(FileInfo)
        XCM_INTERFACE_ENTRY(xd::IFileInfo)
    XCM_END_INTERFACE_MAP()

public:

    FileInfo();
    virtual ~FileInfo() { }

    const std::wstring& getName();
    int getType();
    int getFormat();
    unsigned int getFlags();
    const std::wstring& getMimeType();
    long long getSize();
    xd::rowpos_t getRowCount();
    bool isMount();
    const std::wstring& getPrimaryKey();
    const std::wstring& getObjectId();
    const std::wstring& getUrl();

public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring primary_key;
    std::wstring object_id;
    std::wstring url;
    int type;
    int format;
    int flags;
    long long size;
    xd::rowpos_t row_count;
    bool is_mount;
};


}; // namespace xdcommon



#endif

