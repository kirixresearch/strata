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


class FileInfo : public tango::IFileInfo
{
    XCM_CLASS_NAME("tango.FileInfo")
    XCM_BEGIN_INTERFACE_MAP(FileInfo)
        XCM_INTERFACE_ENTRY(tango::IFileInfo)
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
    tango::rowpos_t getRowCount();
    bool isMount();
    const std::wstring& getPrimaryKey();
    const std::wstring& getObjectId();

public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring primary_key;
    std::wstring object_id;
    int type;
    int format;
    int flags;
    long long size;
    tango::rowpos_t row_count;
    bool is_mount;
};


}; // namespace xdcommon



#endif

