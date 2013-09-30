/*!
 *
 * Copyright (c) 2005-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-13
 *
 */


#ifndef __XDCOMMON_DBENTRY_H
#define __XDCOMMON_DBENTRY_H


class DatabaseEntry : public xd::IDatabaseEntry
{
    XCM_CLASS_NAME("xd.DatabaseEntry")
    XCM_BEGIN_INTERFACE_MAP(DatabaseEntry)
        XCM_INTERFACE_ENTRY(xd::IDatabaseEntry)
    XCM_END_INTERFACE_MAP()

public:

    DatabaseEntry();
    void setName(const std::wstring& name);
    void setDescription(const std::wstring& desc);
    std::wstring getName();
    std::wstring getDescription();

private:
    
    std::wstring m_name;
    std::wstring m_desc;
};



#endif

