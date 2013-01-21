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
    const std::wstring& getMimeType();
    tango::tango_int64_t getSize();
    bool isMount();
    const std::wstring& getPrimaryKey();

public:

    std::wstring name;
    std::wstring mime_type;
    std::wstring primary_key;
    int type;
    int format;
    tango::tango_int64_t size;
    bool is_mount;
};


}; // namespace xdcommon



#endif

