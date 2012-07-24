/*!
 *
 * Copyright (c) 2005-2011, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  XD Database Library
 * Author:   Benjamin I. Williams
 * Created:  2005-01-13
 *
 */


#ifndef __XDCOMMON_DBENTRY_H
#define __XDCOMMON_DBENTRY_H


class DatabaseEntry : public tango::IDatabaseEntry
{
    XCM_CLASS_NAME("tango.DatabaseEntry")
    XCM_BEGIN_INTERFACE_MAP(DatabaseEntry)
        XCM_INTERFACE_ENTRY(tango::IDatabaseEntry)
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

